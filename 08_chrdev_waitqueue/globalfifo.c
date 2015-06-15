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
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALFIFO_SIZE		0x10	/*ȫ���ڴ��С4K*/
#define MEM_CLEAR			0x01	/*����ȫ���ڴ�*/
#define GLOBALFIFO_MAJOR	250		/*Ԥ��globalfifo�����豸��*/
#define GLOBALFIFO_COUNT	2		/*globalfifo�豸�ĸ���*/

static int globalfifo_major = GLOBALFIFO_MAJOR;
static struct class  *globalfifo_class;

/*globalfifo�豸�ṹ��*/
struct globalfifo_dev
{
	struct cdev cdev;		/*cdev�ṹ��*/
	unsigned char mem[GLOBALFIFO_SIZE];	/*ȫ���ڴ�*/
	unsigned int current_len;		/*fifo ����Ч����*/
	struct semaphore sem;			/*���������õ��ź���*/
	wait_queue_head_t r_wait;		/*�������õĵȴ�����ͷ*/
	wait_queue_head_t w_wait;	/*����д�õĵȴ�����ͷ*/
};

struct globalfifo_dev *globalfifo_devp;

static void globalfifo_setup_cdev(struct globalfifo_dev *dev, int index);

static int globalfifo_open(struct inode *inode, struct file *filp)
{
	/*���豸�ṹ��ָ�븳ֵ���ļ�˽������ָ��*/
	struct globalfifo_dev * dev;

	printk("minor = %d", MINOR(inode->i_rdev));

	dev = container_of(inode->i_cdev, struct globalfifo_dev, cdev);	
	filp->private_data = dev;
	return 0;
}

static int globalfifo_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_INFO "pre read %d\n", count);

	while(dev->current_len == 0)
	{
		if(filp->f_flags & O_NONBLOCK)
			return - EAGAIN;
		wait_event_interruptible(dev->r_wait, !(dev->current_len == 0));
	}

	if(down_interruptible(&dev->sem))		/*��ȡ�ź���*/
		return -ERESTARTSYS;

	/*�����ͻ�ȡ��Ч�Ķ�����*/
	if(count > dev->current_len)
		count = dev->current_len;

	/*�ں˿ռ� -> �û��ռ�*/
	if(copy_to_user(buf, (void *)(dev->mem), count))
	{
		up(&dev->sem);
		ret = - EFAULT;
	}
	else
	{
		dev->current_len -= count;
		memcpy(dev->mem, dev->mem + count, dev->current_len);
		ret = count;

		printk(KERN_INFO "read %d byte(s)\n", count);
	}

	up(&dev->sem);

	wake_up_interruptible(&dev->w_wait);

	return ret;
}

static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data;

	printk(KERN_INFO "pre write %d\n", count);

	while(dev->current_len == GLOBALFIFO_SIZE)
	{
		if(filp->f_flags & O_NONBLOCK)
			return - EAGAIN;
		wait_event_interruptible(dev->w_wait, !(dev->current_len == GLOBALFIFO_SIZE));
	}
	

	if(down_interruptible(&dev->sem))		/*��ȡ�ź���*/
		return -ERESTARTSYS;

		/*�����ͻ�ȡ��Ч�Ķ�����*/
	if(count + dev->current_len > GLOBALFIFO_SIZE)
		count = GLOBALFIFO_SIZE - dev->current_len;

	/*�û��ռ�->  �ں˿ռ�**/
	if(copy_from_user((void *)(dev->mem + dev->current_len), buf, count))
	{
		up(&dev->sem);		
		ret = - EFAULT;
	}
	else
	{
		dev->current_len += count;
		ret = count;

		printk(KERN_INFO "written %d byte(s)\n", count);
	}

	up(&dev->sem);

	wake_up_interruptible(&dev->r_wait);

	return ret;
}

static long globalfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalfifo_dev *dev = filp->private_data;
	
	switch(cmd){
	case MEM_CLEAR:
		if(down_interruptible(&dev->sem))		/*��ȡ�ź���*/
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

static const struct file_operations globalfifo_fops= {
	.owner = THIS_MODULE,
	.open = globalfifo_open,
	.release = globalfifo_release,
	.read = globalfifo_read,
	.write = globalfifo_write,
	.unlocked_ioctl = globalfifo_ioctl,
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

static int __init globalfifo_init(void)
{
	int result;
	dev_t devno = MKDEV(globalfifo_major, 0);

	/*�����ַ��豸��������*/
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
	if(!globalfifo_devp)	/*�����ڴ�ʧ��*/
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

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, GLOBALFIFO_COUNT);
	return result;
}

static void __exit globalfifo_exit(void)
{
	cdev_del(&(globalfifo_devp[0].cdev));	/*ɾ��cdev�ṹ*/
	cdev_del(&(globalfifo_devp[1].cdev));
	device_destroy(globalfifo_class, MKDEV(globalfifo_major, 0));
	device_destroy(globalfifo_class, MKDEV(globalfifo_major, 1));
	class_destroy(globalfifo_class);
	kfree(globalfifo_devp);
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), GLOBALFIFO_COUNT);		/*ע���豸����*/
	printk("globalfifo exit.\n");
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);
module_param(globalfifo_major, int, S_IRUGO);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple chrdev driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest driver");

