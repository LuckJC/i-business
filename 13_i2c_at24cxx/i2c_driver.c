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
#include <linux/i2c.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <mach/gpio-nrs.h>
#include <mach/gpio-fns.h>

#define AT24CXX_SIZE 	2048

struct at24cxx_decs
{
	unsigned int major;
	struct class *m_class;
	struct i2c_client *client;
};

struct at24cxx_decs at24cxx_dec;

ssize_t at24cxx_read (struct file *filp, char __user *buf, size_t count, loff_t *offs)
{
	struct i2c_client *client = at24cxx_dec.client;
	unsigned char data[count];
	struct i2c_msg msg[2];

	//printk(KERN_INFO "read client address: 0x%02x\n", client->addr);

	//if(count != 1)
		//return - EFAULT;
	
	/* 1. 定义消息 */
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = (unsigned char *)offs;
	msg[0].len = 1;


	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = count;

	i2c_transfer(client->adapter, msg, 2);

	if(copy_to_user(buf, data, count))
	{
		return - EFAULT;
	}

	*offs += count;
	
	return count;
}

ssize_t at24cxx_write (struct file *filp, const char __user *buf, size_t count, loff_t *offs)
{
	struct i2c_client *client = at24cxx_dec.client;
	unsigned char data[count + 1];
	struct i2c_msg msg[1];

	//printk(KERN_INFO "write client address: 0x%02x\n", client->addr);

	//if(count != 1)
		//return - EFAULT;

	data[0] = (unsigned char)*offs;
	if(copy_from_user(data + 1, buf, count))
	{
		return - EFAULT;
	}
	
	/* 1. 定义消息 */
	msg[0].addr = client->addr;
	msg[0].buf = data;
	msg[0].len = count + 1;
	msg[0].flags = 0;

	i2c_transfer(client->adapter, msg, 1);

	*offs += count;
	
	return count;
}

loff_t at24cxx_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret;
	switch(orig){
	case 0:	/*文件开始*/
		if(offset < 0 || (unsigned int)offset > AT24CXX_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:	/*文件当前位置*/
		if(filp->f_pos + offset < 0 || filp->f_pos + offset  > AT24CXX_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += (unsigned int)offset;
		ret = filp->f_pos;
		break;\
	case 2:	/*文件结束*/
		if(offset > 0 || offset + AT24CXX_SIZE<  0)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += AT24CXX_SIZE + offset;
		ret = filp->f_pos;
		break;
	default:
		ret = - EINVAL;
		break;
	}

	return ret;
}

static struct file_operations at24cxx_fops = 
{
	//.open = at24cxx_open,
	.llseek = at24cxx_llseek,
	.read = at24cxx_read,
	.write = at24cxx_write,
	//.release = at24cxx_close
};

/* 探测函数 */
static int at24cxx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc;

	printk(KERN_INFO "at24cxx_probe\n");
	/* 1. 判断I2C总线是否支持该设备 */
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev, "i2c bus does not support the at24cxx\n");
		rc = -ENODEV;
		goto exit;
	}
	at24cxx_dec.client = client;

	/* 2. 注册字符设备驱动程序*/
	rc = register_chrdev(0, "at24cxx", &at24cxx_fops);
	if(rc == 0)
		goto exit;
	at24cxx_dec.major= rc;

	at24cxx_dec.m_class = class_create(THIS_MODULE, "at24cxx"); 
	if(IS_ERR(at24cxx_dec.m_class))
	{
		goto exit1;
	}
	device_create(at24cxx_dec.m_class, NULL, MKDEV(at24cxx_dec.major, 0), NULL, "at24cxx%d", 0);

	return 0;
	
 exit:
	return rc;

exit1:
	unregister_chrdev(at24cxx_dec.major, "at24cxx");
	return - EINVAL;
}

/* 移除函数 */
static int at24cxx_remove(struct i2c_client *client)
{
	/*注销字符设备*/
	printk(KERN_INFO "at24cxx_remove\n");
	unregister_chrdev(at24cxx_dec.major, "at24cxx");
	device_destroy(at24cxx_dec.m_class, MKDEV(at24cxx_dec.major, 0));
	class_destroy(at24cxx_dec.m_class);
	
	return 0;
}

/* 支持的设备 */
static const struct i2c_device_id at24cxx_id[] = {
	{ "at24cxx", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, at24cxx_id);


/* 分配一个i2c_driver */
static struct i2c_driver at24cxx_driver = {
	.driver = {
		.name = "at24cxx",
	},
	.probe = at24cxx_probe,
	.remove = at24cxx_remove,
	.id_table = at24cxx_id,
};

static int __init at24cxx_init(void)
{
	/* 注册一个i2c_driver */
	return i2c_add_driver(&at24cxx_driver);
}

static void __exit at24cxx_exit(void)
{
	/*注销平台驱动*/
	i2c_del_driver(&at24cxx_driver);
}

module_init(at24cxx_init);
module_exit(at24cxx_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple i2c driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest i2c driver");


