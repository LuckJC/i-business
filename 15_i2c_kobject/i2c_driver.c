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
#include <linux/hwmon-sysfs.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <mach/gpio-nrs.h>
#include <mach/gpio-fns.h>

#define AT24CXX_EEPROM_SIZE 	2048

static ssize_t at24cxx_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct i2c_client *client;
	struct sensor_device_attribute_2 *device_attr;

	printk(KERN_INFO "at24cxx_show\n");
	
	/* 1. 获取i2c_client */
	client = to_i2c_client(dev);

	/* 2. 获取设备属性 */
	device_attr = to_sensor_dev_attr_2(attr);

	/* 3.读取数据*/
	i2c_smbus_read_i2c_block_data(client, device_attr->index, device_attr->nr, buf);
	
	return device_attr->nr;
}


static ssize_t at24cxx_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct i2c_client *client;
	struct sensor_device_attribute_2 *device_attr;

	printk(KERN_INFO "at24cxx_store\n");
	
	/* 1. 获取i2c_client */
	client = to_i2c_client(dev);

	/* 2. 获取设备属性 */
	device_attr = to_sensor_dev_attr_2(attr);

	/* 3.读取数据*/
	i2c_smbus_write_i2c_block_data(client, device_attr->index, device_attr->nr, buf);
	
	return device_attr->nr;
}

/*
 * Simple register attributes
 */
static SENSOR_DEVICE_ATTR_2(four, S_IRUGO | S_IWUSR, at24cxx_show,
			    at24cxx_store, 4, 0);

static const struct attribute_group at24cxx_group = {
	.attrs = (struct attribute *[]) {
		&sensor_dev_attr_four.dev_attr.attr,
		NULL,
	},
};

ssize_t at24cxx_eeprom_read(struct kobject *kobj, struct bin_attribute *attr,
			char *buffer, loff_t pos, size_t count)
{
	struct i2c_client *client;
	
	/* 1. 获取i2c_client */
	client = kobj_to_i2c_client(kobj);

	/* 2.读取数据*/
	i2c_smbus_read_i2c_block_data(client, pos, count, buffer);

	return count;
}

ssize_t at24cxx_eeprom_write(struct kobject *kobj, struct bin_attribute *attr,
			 char *buffer, loff_t pos, size_t count)
{
	struct i2c_client *client;
	
	/* 1. 获取i2c_client */
	client = kobj_to_i2c_client(kobj);

	/* 2.写数据*/
	i2c_smbus_write_i2c_block_data(client, pos, count, buffer);

	return count;
}

static struct bin_attribute at24cxx_eeprom_attr = {
	.attr = {
		.name = "eeprom",
		.mode = S_IRUGO | S_IWUSR,
	},
	.size = AT24CXX_EEPROM_SIZE,
	.read = at24cxx_eeprom_read,
	.write = at24cxx_eeprom_write,
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

	/* 2. 创建sysfs下的二进制文件 */
	rc = sysfs_create_group(&client->dev.kobj, &at24cxx_group);
	if (rc)
		goto exit;

	rc = sysfs_create_bin_file(&client->dev.kobj, &at24cxx_eeprom_attr);
	if (rc)
		goto exit_bin_attr;

	return 0;

 exit_bin_attr:
	sysfs_remove_group(&client->dev.kobj, &at24cxx_group);
 exit:
	return rc;
}

/* 移除函数 */
static int at24cxx_remove(struct i2c_client *client)
{
	printk(KERN_INFO "at24cxx_remove\n");
	/*删除sysfs下的二进制文件*/
	sysfs_remove_bin_file(&client->dev.kobj, &at24cxx_eeprom_attr);
	sysfs_remove_group(&client->dev.kobj, &at24cxx_group);
	
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


