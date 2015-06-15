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

void	gpiokey_release(struct device *dev)
{
	printk("gpiokey release.\n");
}

struct platform_device gpiokey_device = 
{
	.name = "s3c-keys",
	.dev = 
	{
		.release = gpiokey_release,
	}
};

static int __init gpiokey_init(void)
{
	return platform_device_register(&gpiokey_device);
}

static void __exit gpiokey_exit(void)
{
	platform_device_unregister(&gpiokey_device);
}

module_init(gpiokey_init);
module_exit(gpiokey_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A inputdev module");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a inputdev module");

