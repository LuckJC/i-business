#include <linux/init.h>
#include <linux/module.h>

static int hello_init(void)
{
	printk(KERN_INFO "Hello world enter\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_INFO "Hello world exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple hello world module");
MODULE_ALIAS("a simplest module");

