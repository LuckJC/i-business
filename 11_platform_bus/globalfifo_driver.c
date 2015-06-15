#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALFIFO_SIZE		0x10	/*全局内存大小4K*/
#define MEM_CLEAR			0x01	/*清零全局内存*/
#define GLOBALFIFO_MAJOR	250		/*预设globalfifo的主设备号*/
#define GLOBALFIFO_COUNT	2		/*globalfifo设备的个数*/

static int globalfifo_major = GLOBALFIFO_MAJOR;
static struct class  *globalfifo_class;

/*globalfifo设备结构体*/
struct globalfifo_dev
{
	struct cdev cdev;		/*cdev结构体*/
	unsigned char mem[GLOBALFIFO_SIZE];	/*全局内存*/
	unsigned int current_len;		/*fifo 的有效长度*/
	struct semaphore sem;			/*并发控制用的信号量*/
	wait_queue_head_t r_wait;		/*阻塞读用的等待队列头*/
	wait_queue_head_t w_wait;	/*阻塞写用的等待队列头*/
	struct fasync_struct  *async_queue;		/*异步通知结构体*/
};

struct globalfifo_dev *globalfifo_devp;

static void globalfifo_setup_cdev(struct globalfifo_dev *dev, int index);

static int globalfifo_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	struct globalfifo_dev * dev;

	printk("minor = %d", MINOR(inode->i_rdev));

	dev = container_of(inode->i_cdev, struct globalfifo_dev, cdev);	
	filp->private_data = dev;
	return 0;
}

static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_INFO "pre read %d\n", count);

	if(dev->current_len == 0)
	{
		if(filp->f_flags & O_NONBLOCK)
			return - EAGAIN;
		ret = wait_event_interruptible(dev->r_wait, !(dev->current_len == 0));
		if(ret != 0)
			return - ERESTARTSYS;
	}

	if(down_interruptible(&dev->sem))		/*获取信号量*/
		return -ERESTARTSYS;

	/*分析和获取有效的读长度*/
	if(count > dev->current_len)
		count = dev->current_len;

	/*内核空间 -> 用户空间*/
	if(copy_to_user(buf, (void *)(dev->mem), count))
	{
		ret = - EFAULT;
	}
	else
	{
		dev->current_len -= count;
		memcpy(dev->mem, dev->mem + count, dev->current_len);
		ret = count;
		
		printk(KERN_INFO "read %d byte(s)\n", count);	
		/*产生异步写信号*/
		if(dev->async_queue)
			kill_fasync(&dev->async_queue, SIGIO, POLL_OUT);
		wake_up_interruptible(&dev->w_wait);
	}

	up(&dev->sem);

	return ret;
}

static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_INFO "pre write %d\n", count);

	if(dev->current_len == GLOBALFIFO_SIZE)
	{
		if(filp->f_flags & O_NONBLOCK)
			return - EAGAIN;
		ret = wait_event_interruptible(dev->r_wait, !(dev->current_len == 0));
		if(ret != 0)
			return - ERESTARTSYS;
	}

	if(down_interruptible(&dev->sem))		/*获取信号量*/
		return -ERESTARTSYS;

		/*分析和获取有效的读长度*/
	if(count + dev->current_len > GLOBALFIFO_SIZE)
		count = GLOBALFIFO_SIZE - dev->current_len;

	/*用户空间->  内核空间**/
	if(copy_from_user((void *)(dev->mem + dev->current_len), buf, count))
	{	
		ret = - EFAULT;
	}
	else
	{
		dev->current_len += count;
		ret = count;
		
		printk(KERN_INFO "written %d byte(s)\n", count);	
		/*产生异步读信号*/
		if(dev->async_queue)
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

		wake_up_interruptible(&dev->r_wait);
	}

	up(&dev->sem);
	
	return ret;
}

static long globalfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalfifo_dev *dev = filp->private_data;
	
	switch(cmd){
	case MEM_CLEAR:
		if(down_interruptible(&dev->sem))		/*获取信号量*/
			return -ERESTARTSYS;

		memset(dev->mem, 0, GLOBALFIFO_SIZE);
		dev->current_len = 0;
		printk(KERN_INFO "globalfifo is set to zero\n");

		up(&dev->sem);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int globalfifo_fasync (int fd, struct file *filp, int mode)
{
	struct globalfifo_dev *dev = filp->private_data;
	/*将文件加加到异步通知表中*/
	 return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int globalfifo_release(struct inode *inode, struct file *filp)
{
	/*将文件从异步通知列表中删除*/
	globalfifo_fasync(-1, filp, 0);
	return 0;
}

static const struct file_operations globalfifo_fops= {
	.owner = THIS_MODULE,
	.open = globalfifo_open,
	.release = globalfifo_release,
	.read = globalfifo_read,
	.write = globalfifo_write,
	.unlocked_ioctl = globalfifo_ioctl,
	.fasync = globalfifo_fasync,
};

static void globalfifo_setup_cdev(struct globalfifo_dev *dev, int index)
{
	int err, devno = MKDEV(globalfifo_major, index);

	cdev_init(&dev->cdev, &globalfifo_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Erro %d adding globalfifo", err);
}

/*平台驱动入口函数--> 将原来的模块入口函数迁移过来*/
static int globalfifo_probe(struct platform_device *devp)
{
	int result;
	dev_t devno = MKDEV(globalfifo_major, 0);

	/*申请字符设备驱动区域*/
	if(globalfifo_major)
	{
		result = register_chrdev_region(devno, GLOBALFIFO_COUNT, "globalfifo");
	}
	else
	{
		result = alloc_chrdev_region(&devno, 0, GLOBALFIFO_COUNT, "globalfifo");
		globalfifo_major = MAJOR(devno);
	}

	printk(KERN_INFO "globalfifo_major = %d\n", globalfifo_major);

	if(result < 0)
		return result;

	globalfifo_devp = kmalloc(GLOBALFIFO_COUNT * sizeof(struct globalfifo_dev), GFP_KERNEL);
	if(!globalfifo_devp)	/*申请内存失败*/
	{
		result = -ENOMEM;
		goto fail_malloc;
	}

	//memset(globalfifo_devp, 0, GLOBALFIFO_COUNT * sizeof(struct globalfifo_dev));

	globalfifo_setup_cdev(&globalfifo_devp[0], 0);
	globalfifo_setup_cdev(&globalfifo_devp[1], 1);

	globalfifo_class = class_create(THIS_MODULE, "memdev"); 
	device_create(globalfifo_class,NULL, devno, NULL,"globalfifo%d",0);
	device_create(globalfifo_class,NULL, devno + 1, NULL,"globalfifo%d",1);

	globalfifo_devp[0].current_len = 0;
	globalfifo_devp[1].current_len = 0;

	sema_init(&globalfifo_devp[0].sem, 1);
	sema_init(&globalfifo_devp[1].sem, 1);

	init_waitqueue_head(&globalfifo_devp[0].r_wait);
	init_waitqueue_head(&globalfifo_devp[0].w_wait);
	init_waitqueue_head(&globalfifo_devp[1].r_wait);
	init_waitqueue_head(&globalfifo_devp[1].w_wait);

	globalfifo_devp[0].async_queue = NULL;
	globalfifo_devp[1].async_queue = NULL;

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, GLOBALFIFO_COUNT);
	return result;
}

/*平台驱动出口函数--> 将原来的模块出口函数迁移过来*/
static int globalfifo_remove(struct platform_device *devp)
{
	cdev_del(&(globalfifo_devp[0].cdev));	/*删除cdev结构*/
	cdev_del(&(globalfifo_devp[1].cdev));
	device_destroy(globalfifo_class, MKDEV(globalfifo_major, 0));
	device_destroy(globalfifo_class, MKDEV(globalfifo_major, 1));
	class_destroy(globalfifo_class);
	kfree(globalfifo_devp);
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), GLOBALFIFO_COUNT);		/*注销设备区域*/
	printk("globalfifo exit.\n");

	return 0;
}

/*定义平台驱动结构体*/
struct platform_driver globalfifo_platform_driver = 
{
	.probe = globalfifo_probe,
	.remove = globalfifo_remove,
	.driver = 
	{
		.name = "globalfifo",
		.owner = THIS_MODULE,
	}
};

static int __init globalfifo_init(void)
{
	/*注册平台驱动*/
	return platform_driver_register(&globalfifo_platform_driver);
}

static void __exit globalfifo_exit(void)
{
	/*注销平台驱动*/
	platform_driver_unregister(&globalfifo_platform_driver);
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);
module_param(globalfifo_major, int, S_IRUGO);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple platform driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest platform driver");


