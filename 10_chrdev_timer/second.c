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
#include <linux/timer.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

struct second_dev
{
	atomic_t counter;		/*一共经历了多少秒*/
	struct timer_list s_timer;	/*设备使用的定时器*/		
};

static int second_major = 250;

static struct second_dev *second_devp;
static struct class *second_class;

static void second_handler(unsigned long arg)
{
	struct second_dev *dev = (struct second_dev *)arg;
	mod_timer(&dev->s_timer, jiffies + HZ);

	atomic_inc(&dev->counter);

	printk(KERN_INFO "current jiffies is %d", jiffies);
}

static int second_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	struct second_dev * dev;

	printk("minor = %d", MINOR(inode->i_rdev));

	dev = second_devp;
	filp->private_data = dev;

	atomic_set(&dev->counter, 0);
	/*1. 初始化定时器*/
	init_timer(&dev->s_timer);
	dev->s_timer.function = second_handler;
	dev->s_timer.data = (unsigned long)second_devp;
	dev->s_timer.expires = jiffies + HZ;
	/*2.  添加一个内核定时器*/
	add_timer(&dev->s_timer);

	
	return 0;
}

static ssize_t second_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int counter;
	struct second_dev * dev = filp->private_data;

	counter = atomic_read(&dev->counter);
	//printk(KERN_INFO "counter = %d\n", counter);

	if(put_user(counter, (int *)buf))
		return -EFAULT;
	else
		return sizeof(unsigned int);
}

static int second_release(struct inode *inode, struct file *filp)
{
	/*将文件从异步通知列表中删除*/

	/*4.1 删除内核定时器*/
	del_timer(&second_devp->s_timer);
	
	return 0;
}


struct file_operations second_fops = 
{
	.open = second_open,
	.read = second_read,
	.release = second_release,
};

static int __init second_init(void)
{
	int ret;
	/*1、注册一个字符设备*/
	ret = register_chrdev(second_major, "second_dev", &second_fops);
	if(ret < 0)
	{
		printk(KERN_INFO "register_chrdev failed.\n");
		return ret;
	}
	if(!second_major)	//判断是必须的，如果second_major本身有值，则ret正确返回0
		second_major = ret;

	/*2、定一个类*/
	second_class = class_create(THIS_MODULE, "second_dev");
	if(IS_ERR(second_class)) 
	{
		printk(KERN_INFO"Err: failed in creating class.\n");
		goto out;
	}
	
	/*3、在类下面创建设备*/
	device_create(second_class, NULL, MKDEV(second_major, 0), NULL,  "second_dev%d", 0);

	/*4、硬件相关资源初始化*/
	/*4.1 初始化设备结构体内存*/
	second_devp = kmalloc(sizeof(struct second_dev), GFP_KERNEL);
	if(!second_devp)
	{
		ret = - ENOMEM;
		goto out1;
	}
	atomic_set(&second_devp->counter, 0);

	return 0;

	/*5、错误处理*/
out1:
	device_destroy(second_class, MKDEV(second_major, 0));
	class_destroy(second_class);
out:
	unregister_chrdev(second_major, "second_dev");
	return  -1;
}

static void __exit second_exit(void)
{
	/*1、注销一个字符设备*/
	unregister_chrdev(second_major, "second_dev");
	/*2、销毁类下的设备*/
	device_destroy(second_class, MKDEV(second_major, 0));
	/*3、销毁类*/
	class_destroy(second_class);
	/*4、硬件相关资源释放*/
}

module_init(second_init);
module_exit(second_exit);
module_param(second_major, int, S_IRUGO);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A second device driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a second device driver");

