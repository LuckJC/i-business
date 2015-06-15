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
	int pin;			/*������*/
	char *name;		/*��������*/
	unsigned int irq;	/*�жϺ�*/
	int code;			/*�����ı���*/	
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
/*1. ��ȡ�����ṹ��*/
	struct pin_descs *pin;
	int val = 0;
	
	pin = (struct pin_descs *)data;
	if(pin == NULL)
		return ;
	
	/*2. ȷ������ֵ(���¡��ɿ�)*/
	val= s3c2410_gpio_getpin(pin->pin);

	/*3. ��ϵͳ���水��ֵ*/
	/*3.1 ���水���¼�*/
	input_report_key(gpiokey_dev, pin->code, !val);
	/*3.2 ����ͬ���¼�*/
	input_sync(gpiokey_dev);
}


/*ƽ̨������ں���--> ��ԭ����ģ����ں���Ǩ�ƹ���*/
static int globalfifo_probe(struct platform_device *devp)
{
	int ret = 0;
	/*1. ����һ��input_dev*/
	gpiokey_dev = input_allocate_device();
	if(IS_ERR(gpiokey_dev))
	{
		printk(KERN_ERR "ERR:input_allocate_device\n");
		return -EFAULT;
	}

	/*2. ����input_dev*/
	/*2.1 ���Բ��������¼�*/
	set_bit(EV_KEY, gpiokey_dev->evbit);
	set_bit(EV_REP, gpiokey_dev->evbit);  
	set_bit(EV_SYN, gpiokey_dev->evbit);
	/*2.2 ���Բ�����Щ ����*/
	set_bit(KEY_J, gpiokey_dev->keybit);
	set_bit(KEY_V, gpiokey_dev->keybit);
	set_bit(KEY_C, gpiokey_dev->keybit);
	set_bit(KEY_ENTER, gpiokey_dev->keybit);
	set_bit(KEY_LEFTSHIFT, gpiokey_dev->keybit);
	set_bit(KEY_BACKSPACE, gpiokey_dev->keybit);

	gpiokey_dev->name = "jovec";
	
	/*3. ע��һ��input_dev*/
	ret = input_register_device(gpiokey_dev);
	if(ret != 0)
	{
		printk(KERN_ERR "ERR:input_register_device\n");
		goto out1;
	}

	/*4. Ӳ����ز���*/
	/* 4.1 �����ж� */
	//EINT8   EINT11  EINT13  EINT14  EINT15  EINT19
	//GPG0  GPG3  GPG5  GPG6  GPG7  GPG11
	int i =0;
	for(i = 0; i < 6; i++)
	{
		request_irq((pins+i)->irq, buttons_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, (pins+i)->name, pins + i);
	}
	
	/*4.2 ���÷�����ʱ��*/
	init_timer(&button_timer); 
	button_timer.function = timer_handler;
	button_timer.expires = 0;
	add_timer(&button_timer);
	
	return 0;

out1:
	input_free_device(gpiokey_dev);
	return ret;
}

/*ƽ̨�������ں���--> ��ԭ����ģ����ں���Ǩ�ƹ���*/
static int globalfifo_remove(struct platform_device *devp)
{
	/*1. ע��һ��input_dev*/
	input_unregister_device(gpiokey_dev);

	/*2. �ͷ�һ��input_dev*/
	input_free_device(gpiokey_dev);

	/*3. Ӳ����ز���*/
	/*3.1 �ͷ��ж�*/
	int i;
	for(i = 0; i < 6; i++)
	{
		free_irq((pins + i)->irq, (pins + i));
	}

	del_timer(&button_timer);
	
	return 0;
}

/*����ƽ̨�����ṹ��*/
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
	/*ע��ƽ̨����*/
	return platform_driver_register(&globalfifo_platform_driver);
}

static void __exit gpiokey_exit(void)
{
	/*ע��ƽ̨����*/
	platform_driver_unregister(&globalfifo_platform_driver);
}

module_init(gpiokey_init);
module_exit(gpiokey_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple gpiokey driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest gpiokey driver");


