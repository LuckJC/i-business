#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALMEM_MAJOR	250

int globalmem_major = GLOBALMEM_MAJOR;

static int __init globalmem_init(void)
{
	int result;
	dev_t devno = MKDEV(globalmem_major, 0);

	/*申请字符设备驱动区域*/
	if(globalmem_major)
	{
		result = register_chrdev_region(devno, 1, "globalmem");
	}
	else
	{
		result = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}

	if(result < 0)
		return result;

	printk(KERN_INFO "register chrdev region.\n");
	printk(KERN_INFO "globalmem_major  = %d\n", globalmem_major);

	return 0;
}

static void __exit globalmem_exit(void)
{
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);		/*注销设备区域*/
	printk(KERN_INFO "unregister chrdev region.\n");
	printk(KERN_INFO "globalmem_major  = %d\n", globalmem_major);
}

module_init(globalmem_init);
module_exit(globalmem_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple chrdev driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest driver");

