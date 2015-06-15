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
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <mach/gpio-nrs.h>
#include <mach/gpio-fns.h>


struct pin_descs
{
	int pin;			/*按键号*/
	char *name;		/*按键名字*/
	unsigned int irq;	/*中断号*/
	int code;			/*按键的编码*/	
};

//EINT8   EINT11  EINT13  EINT14  EINT15  EINT19
//GPG0  GPG3  GPG5  GPG6  GPG7  GPG11
struct pin_descs pins[6] =
{
	{S3C2410_GPG(0),	"S0", IRQ_EINT8,	KEY_J			},
	{S3C2410_GPG(3), 	"S1", IRQ_EINT11,	KEY_V			},
	{S3C2410_GPG(5), 	"S2", IRQ_EINT13,	KEY_C			},
	{S3C2410_GPG(6), 	"S3", IRQ_EINT14,	KEY_ENTER		},
	{S3C2410_GPG(7), 	"S4", IRQ_EINT15,	KEY_LEFTSHIFT	},
	{S3C2410_GPG(11), 	"S5", IRQ_EINT19,	KEY_BACKSPACE	}
};

static struct input_dev *gpiokey_dev;
struct timer_list button_timer;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	button_timer.data = (unsigned long)dev_id; 
	mod_timer(&button_timer, jiffies + HZ / 200);	

	return IRQ_HANDLED;
}

static void timer_handler(unsigned long data)
{
/*1. 获取按键结构体*/
	struct pin_descs *pin;
	int val = 0;
	
	pin = (struct pin_descs *)data;
	if(pin == NULL)
		return ;
	
	/*2. 确定按键值(按下、松开)*/
	val= s3c2410_gpio_getpin(pin->pin);

	/*3. 向系统报告按键值*/
	/*3.1 报告按键事件*/
	input_report_key(gpiokey_dev, pin->code, !val);
	/*3.2 报告同步事件*/
	input_sync(gpiokey_dev);
}


/*平台驱动入口函数--> 将原来的模块入口函数迁移过来*/
static int globalfifo_probe(struct platform_device *devp)
{
	int ret = 0;
	/*1. 分配一个input_dev*/
	gpiokey_dev = input_allocate_device();
	if(IS_ERR(gpiokey_dev))
	{
		printk(KERN_ERR "ERR:input_allocate_device\n");
		return -EFAULT;
	}

	/*2. 设置input_dev*/
	/*2.1 可以产生哪类事件*/
	set_bit(EV_KEY, gpiokey_dev->evbit);
	set_bit(EV_REP, gpiokey_dev->evbit);  
	set_bit(EV_SYN, gpiokey_dev->evbit);
	/*2.2 可以产生哪些 按键*/
	set_bit(KEY_J, gpiokey_dev->keybit);
	set_bit(KEY_V, gpiokey_dev->keybit);
	set_bit(KEY_C, gpiokey_dev->keybit);
	set_bit(KEY_ENTER, gpiokey_dev->keybit);
	set_bit(KEY_LEFTSHIFT, gpiokey_dev->keybit);
	set_bit(KEY_BACKSPACE, gpiokey_dev->keybit);

	gpiokey_dev->name = "jovec";
	
	/*3. 注册一个input_dev*/
	ret = input_register_device(gpiokey_dev);
	if(ret != 0)
	{
		printk(KERN_ERR "ERR:input_register_device\n");
		goto out1;
	}

	/*4. 硬件相关操作*/
	/* 4.1 申请中断 */
	//EINT8   EINT11  EINT13  EINT14  EINT15  EINT19
	//GPG0  GPG3  GPG5  GPG6  GPG7  GPG11
	int i =0;
	for(i = 0; i < 6; i++)
	{
		request_irq((pins+i)->irq, buttons_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, (pins+i)->name, pins + i);
	}
	
	/*4.2 设置防抖定时器*/
	init_timer(&button_timer); 
	button_timer.function = timer_handler;
	button_timer.expires = 0;
	add_timer(&button_timer);
	
	return 0;

out1:
	input_free_device(gpiokey_dev);
	return ret;
}

/*平台驱动出口函数--> 将原来的模块出口函数迁移过来*/
static int globalfifo_remove(struct platform_device *devp)
{
	/*1. 注销一个input_dev*/
	input_unregister_device(gpiokey_dev);

	/*2. 释放一个input_dev*/
	input_free_device(gpiokey_dev);

	/*3. 硬件相关操作*/
	/*3.1 释放中断*/
	int i;
	for(i = 0; i < 6; i++)
	{
		free_irq((pins + i)->irq, (pins + i));
	}

	del_timer(&button_timer);
	
	return 0;
}

/*定义平台驱动结构体*/
struct platform_driver globalfifo_platform_driver = 
{
	.probe = globalfifo_probe,
	.remove = globalfifo_remove,
	.driver = 
	{
		.name = "s3c-keys",
		.owner = THIS_MODULE,
	}
};

static int __init gpiokey_init(void)
{
	/*注册平台驱动*/
	return platform_driver_register(&globalfifo_platform_driver);
}

static void __exit gpiokey_exit(void)
{
	/*注销平台驱动*/
	platform_driver_unregister(&globalfifo_platform_driver);
}

module_init(gpiokey_init);
module_exit(gpiokey_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple gpiokey driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest gpiokey driver");


