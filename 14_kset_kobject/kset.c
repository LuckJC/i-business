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


static int mkset_filter(struct kset *kset, struct kobject *kobj)
{
	printk(KERN_INFO "mkset_filter: %s\n", kobj->name);
	
	return 1;
}

	
static const char *mkset_name(struct kset *kset, struct kobject *kobj)
{
	printk(KERN_INFO "mkset_name: %s\n", kobj->name);
	return kobj->name;
}

static int mkset_uevent(struct kset *kset, struct kobject *kobj, struct kobj_uevent_env *env)
{
	int i = 0;
	printk(KERN_INFO "mkset_uevent: %s\n", kobj->name);

	for(i = 0; i < env->envp_idx; i++)
	{
		printk(KERN_INFO "env: %s\n", env->envp[i]);
	}
	
	return 0;
}

static struct kset_uevent_ops  mkset_ops = {
	.filter = mkset_filter,
	.name = mkset_name,
	.uevent = mkset_uevent
};

/* 定义一个kset */
static struct kset *kset_p;
static struct kset kset_c;

static int __init mkset_init(void)
{
	/* 创建并注册一个kset */
	kset_p = kset_create_and_add("zjj", &mkset_ops, NULL);
	if(kset_p == NULL)
		return - EFAULT;
	
	kobject_set_name(&kset_c.kobj, "hc");
	kset_c.kobj.kset = kset_p;
	kset_register(&kset_c);
	
	return 0;
}

static void __exit mkset_exit(void)
{
	/* 注销kset */
	kset_unregister(&kset_c);
	kset_unregister(kset_p);
}

module_init(mkset_init);
module_exit(mkset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_DESCRIPTION("A kset test module");
