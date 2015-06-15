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
#include <linux/platform_device.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

void	globalfifo_release(struct device *dev)
{
	printk("globalfifo release.\n");
}

struct platform_device globalfifo_device = 
{
	.name = "globalfifo",
	.dev = 
	{
		.release = globalfifo_release,
	}
};

static int __init globalfifo_init(void)
{
	return platform_device_register(&globalfifo_device);
}

static void __exit globalfifo_exit(void)
{
	platform_device_unregister(&globalfifo_device);
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A global device module");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a global device module");

