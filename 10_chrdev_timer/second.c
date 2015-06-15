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
	atomic_t counter;		/*һ�������˶�����*/
	struct timer_list s_timer;	/*�豸ʹ�õĶ�ʱ��*/		
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
	/*���豸�ṹ��ָ�븳ֵ���ļ�˽������ָ��*/
	struct second_dev * dev;

	printk("minor = %d", MINOR(inode->i_rdev));

	dev = second_devp;
	filp->private_data = dev;

	atomic_set(&dev->counter, 0);
	/*1. ��ʼ����ʱ��*/
	init_timer(&dev->s_timer);
	dev->s_timer.function = second_handler;
	dev->s_timer.data = (unsigned long)second_devp;
	dev->s_timer.expires = jiffies + HZ;
	/*2.  ���һ���ں˶�ʱ��*/
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
	/*���ļ����첽֪ͨ�б���ɾ��*/

	/*4.1 ɾ���ں˶�ʱ��*/
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
	/*1��ע��һ���ַ��豸*/
	ret = register_chrdev(second_major, "second_dev", &second_fops);
	if(ret < 0)
	{
		printk(KERN_INFO "register_chrdev failed.\n");
		return ret;
	}
	if(!second_major)	//�ж��Ǳ���ģ����second_major������ֵ����ret��ȷ����0
		second_major = ret;

	/*2����һ����*/
	second_class = class_create(THIS_MODULE, "second_dev");
	if(IS_ERR(second_class)) 
	{
		printk(KERN_INFO"Err: failed in creating class.\n");
		goto out;
	}
	
	/*3���������洴���豸*/
	device_create(second_class, NULL, MKDEV(second_major, 0), NULL,  "second_dev%d", 0);

	/*4��Ӳ�������Դ��ʼ��*/
	/*4.1 ��ʼ���豸�ṹ���ڴ�*/
	second_devp = kmalloc(sizeof(struct second_dev), GFP_KERNEL);
	if(!second_devp)
	{
		ret = - ENOMEM;
		goto out1;
	}
	atomic_set(&second_devp->counter, 0);

	return 0;

	/*5��������*/
out1:
	device_destroy(second_class, MKDEV(second_major, 0));
	class_destroy(second_class);
out:
	unregister_chrdev(second_major, "second_dev");
	return  -1;
}

static void __exit second_exit(void)
{
	/*1��ע��һ���ַ��豸*/
	unregister_chrdev(second_major, "second_dev");
	/*2���������µ��豸*/
	device_destroy(second_class, MKDEV(second_major, 0));
	/*3��������*/
	class_destroy(second_class);
	/*4��Ӳ�������Դ�ͷ�*/
}

module_init(second_init);
module_exit(second_exit);
module_param(second_major, int, S_IRUGO);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A second device driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a second device driver");

