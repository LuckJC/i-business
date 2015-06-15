#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALMEM_SIZE		0x1000	/*全局内存大小4K*/
#define MEM_CLEAR			0x01	/*清零全局内存*/
#define GLOBALMEM_MAJOR	250		/*预设globalmem的主设备号*/
#define GLOBALMEM_COUNT	2		/*globalmem设备的个数*/

static int globalmem_major = GLOBALMEM_MAJOR;

/*globalmem设备结构体*/
struct globalmem_dev
{
	struct cdev cdev;		/*cdev结构体*/
	unsigned char mem[GLOBALMEM_SIZE];	/*全局内存*/
	struct semaphore sem;
};

struct globalmem_dev *globalmem_devp;

static void globalmem_setup_cdev(struct globalmem_dev *dev, int index);

static int globalmem_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	struct globalmem_dev * dev;

	printk("minor = %d", MINOR(inode->i_rdev));

	dev = container_of(inode->i_cdev, struct globalmem_dev, cdev);	
	filp->private_data = dev;
	return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;

	struct globalmem_dev *dev = filp->private_data;

	/*分析和获取有效的读长度*/
	if(p >= GLOBALMEM_SIZE)
		return 0;

	if(down_interruptible(&dev->sem))		/*获取信号量*/
		return -ERESTARTSYS;

	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	/*内核空间 -> 用户空间*/
	if(copy_to_user(buf, (void *)(dev->mem + p), count))
	{
		up(&dev->sem);
		ret = - EFAULT;
	}
	else
	{
		*ppos += count;
		ret = count;

		printk(KERN_INFO "read %d byte(s) from %d\n", count, (int)p);
	}

	up(&dev->sem);

	return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;

	struct globalmem_dev *dev = filp->private_data;

	/*分析和获取有效的读长度*/
	if(p >= GLOBALMEM_SIZE)
		return 0;

	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if(down_interruptible(&dev->sem))		/*获取信号量*/
		return -ERESTARTSYS;

	/*用户空间->  内核空间**/
	if(copy_from_user((void *)(dev->mem + p), buf, count))
	{
		up(&dev->sem);		
		ret = - EFAULT;
	}
	else
	{
		*ppos += count;
		ret = count;

		printk(KERN_INFO "written %d byte(s) from %d\n", count, (int)p);
	}

	up(&dev->sem);

	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret;
	switch(orig){
	case 0:	/*文件开始*/
		if(offset < 0 || (unsigned int)offset > GLOBALMEM_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:	/*文件当前位置*/
		if(filp->f_pos + offset < 0 || filp->f_pos + offset  > GLOBALMEM_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += (unsigned int)offset;
		ret = filp->f_pos;
		break;\
	case 2:	/*文件结束*/
		if(offset > 0 || offset + GLOBALMEM_SIZE<  0)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += GLOBALMEM_SIZE + offset;
		ret = filp->f_pos;
		break;
	default:
		ret = - EINVAL;
		break;
	}

	return ret;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalmem_dev *dev = filp->private_data;
	
	switch(cmd){
	case MEM_CLEAR:
		if(down_interruptible(&dev->sem))		/*获取信号量*/
			return -ERESTARTSYS;

		memset(dev->mem, 0, GLOBALFIFO_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");

		up(&dev->sem);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations globalmem_fops= {
	.owner = THIS_MODULE,
	.open = globalmem_open,
	.release = globalmem_release,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
};

static void globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
	int err, devno = MKDEV(globalmem_major, index);

	cdev_init(&dev->cdev, &globalmem_fops);
	globalmem_devp->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Erro %d adding globalmem", err);
}

static int __init globalmem_init(void)
{
	int result;
	dev_t devno = MKDEV(globalmem_major, 0);

	/*申请字符设备驱动区域*/
	if(globalmem_major)
	{
		result = register_chrdev_region(devno, GLOBALMEM_COUNT, "globalmem");
	}
	else
	{
		result = alloc_chrdev_region(&devno, 0, GLOBALMEM_COUNT, "globalmem");
		globalmem_major = MAJOR(devno);
	}

	printk(KERN_INFO "globalfifo_major = %d\n", globalmem_major);

	if(result < 0)
		return result;

	globalmem_devp = kmalloc(GLOBALMEM_COUNT * sizeof(struct globalmem_dev), GFP_KERNEL);
	if(!globalmem_devp)	/*申请内存失败*/
	{
		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(globalmem_devp, 0, GLOBALMEM_COUNT * sizeof(struct globalmem_dev));

	globalmem_setup_cdev(&globalmem_devp[0], 0);
	globalmem_setup_cdev(&globalmem_devp[1], 1);

	sema_init(&globalmem_devp[0].sem, 1);
	sema_init(&globalmem_devp[1].sem, 1);

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, GLOBALMEM_COUNT);
	return result;
}

static void __exit globalmem_exit(void)
{
	cdev_del(&(globalmem_devp[0].cdev));	/*删除cdev结构*/
	cdev_del(&(globalmem_devp[1].cdev));
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), GLOBALMEM_COUNT);		/*注销设备区域*/
	printk("globalmem exit.\n");
}

module_init(globalmem_init);
module_exit(globalmem_exit);
module_param(globalmem_major, int, S_IRUGO);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple chrdev driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest driver");

