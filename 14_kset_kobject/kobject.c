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
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>


#define OBJ_BUF_SIZE	1024

static char obj_buf[OBJ_BUF_SIZE] = {0};

static void obj_release(struct kobject *kobj)
{
	printk(KERN_INFO "obj_release\n");
}

static ssize_t obj_show(struct kobject *kobj, struct attribute *attr, char *buffer)
{
	printk(KERN_INFO "obj_show\n");

	/* 将信息输出到用户空间 */
	/* 不需要使用copy_to_user */
	strncpy(buffer, obj_buf, strlen(obj_buf));
	
	return strlen(buffer);
}


static ssize_t obj_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t count)
{
	printk(KERN_INFO "obj_store\n");

	/* 将用户空间的信息保存起来 */
	/* 不需要使用copy_from_user */
	strncpy(obj_buf, buffer, count);
	
	return count;
}

/* 1. 定义一个kobject */
struct kobject  obj_koject;

struct attribute obj_attr = 
{
	.name = "zjj",
	.owner = THIS_MODULE,
	.mode = (S_IRUGO | S_IWUGO)
};

struct attribute *obj_attrs[] = 
{
	&obj_attr, NULL
};

struct sysfs_ops obj_sysfs_ops = 
{
	.show = obj_show,
	.store = obj_store
};

struct kobj_type obj_type = 
{
	.release = obj_release,
	.sysfs_ops = &obj_sysfs_ops,
	.default_attrs = obj_attrs
};

static int __init koject_init(void)
{
	/* 1. 初始化一个kobject */
	kobject_set_name(&obj_koject, "obj");
	kobject_init(&obj_koject, &obj_type);

	/* 2. 注册一个kobject */
	kobject_add(&obj_koject, NULL, "jovec");

	return 0;
}

static void __exit kobject_exit(void)
{
	/* 1. 注销一个kobject */
	kobject_del(&obj_koject);
}


module_init(koject_init);
module_exit(kobject_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_DESCRIPTION("A kobject test module");
MODULE_LICENSE("GPL");

