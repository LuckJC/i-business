#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <asm/irq.h>

struct ts_regs
{
	unsigned int adccon;
	unsigned int adctsc;
	unsigned int adccdly;
	unsigned int adcdat0;
	unsigned int adcdat1;
	unsigned int adcupdn;
};

static struct input_dev *s3c_ts_dev;
volatile struct ts_regs *ts_regs;
static struct timer_list ts_timer;

static void wait_pen_down(void)
{
	ts_regs->adctsc = 0xd3;
}

static void wait_pen_up(void)
{
	ts_regs->adctsc = 0x1d3;
}

static void start_adc(void)
{
	//ts_regs->adctsc = (1 << 6) | (1 << 4) | (1 << 3) | (1 << 2);
	ts_regs->adctsc = 0xdc;
	ts_regs->adccon |= 1;
}

static void ts_timer_func(unsigned long time)
{
	if(!(ts_regs->adcdat0 & (1 << 15)))
	{
		start_adc();
	}
}

static irqreturn_t s3c_ts_pen_updown(int irq, void *dev_id)
{
	if(ts_regs->adcdat0 & (1 << 15))	/* pen抬起 */
	{
		//printk("pen is up.\n");
		input_report_key(s3c_ts_dev, BTN_TOUCH, 0);
		input_sync(s3c_ts_dev);
		wait_pen_down();
	}
	else
	{
		//printk("pen is down.\n");
		input_report_key(s3c_ts_dev, BTN_TOUCH, 1);
		input_sync(s3c_ts_dev);
		/* 启动ADC转换 */
		start_adc();
		//wait_pen_up();
	}

	return IRQ_HANDLED;
}

static irqreturn_t s3c_ts_adc_irq(int irq, void *dev_id)
{
	if(ts_regs->adcdat0 & (1 << 15))
	{
		input_report_key(s3c_ts_dev, BTN_TOUCH, 0);
		//input_report_abs(s3c_ts_dev, ABS_PRESSURE, 0);
		input_sync(s3c_ts_dev);
		wait_pen_down();
	}
	else
	{
		//printk("x = %d, y = %d\n", ts_regs->adcdat0 & 0x3ff,  ts_regs->adcdat1 & 0x3ff);
		input_report_key(s3c_ts_dev, BTN_TOUCH, 1);
		input_report_abs(s3c_ts_dev, ABS_X, ts_regs->adcdat0 & 0x3ff);
		input_report_abs(s3c_ts_dev, ABS_Y, ts_regs->adcdat1 & 0x3ff);
		input_report_abs(s3c_ts_dev, ABS_PRESSURE, 1);
		input_sync(s3c_ts_dev);

		mod_timer(&ts_timer, jiffies + HZ / 20);
		wait_pen_up();
		//msleep(500);
		//start_adc();
	}
	
	return IRQ_HANDLED;
}


static int s3c_ts_init(void)
{
	struct clk *adc_clock;
	
	/* 1. 分配input_dev */
	s3c_ts_dev = input_allocate_device();
	
	/* 2. 配置 */
	/*2. 设置input_dev*/
	/*2.1 可以产生哪类事件*/
	set_bit(EV_KEY, s3c_ts_dev->evbit);
	set_bit(EV_ABS, s3c_ts_dev->evbit);
	set_bit(EV_SYN, s3c_ts_dev->evbit);
	/*2.2 可以产生哪些 按键*/
	set_bit(BTN_TOUCH, s3c_ts_dev->keybit);
	input_set_abs_params(s3c_ts_dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(s3c_ts_dev, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(s3c_ts_dev, ABS_PRESSURE, 0, 1, 0, 0);

	/* 3. 注册 */
	input_register_device(s3c_ts_dev);

	/* 4. 硬件相关的初始化 */
	/* 4.1  ioremap */
	ts_regs = ioremap(0x58000000, sizeof(struct ts_regs));
	/* 4.2  配置寄存器 */
	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}
	clk_enable(adc_clock);

	ts_regs->adccon = (1 << 14) | (49 << 6);
	ts_regs->adccdly = 0xffff;
	
	/* 4.3  注册中断 */
	request_irq(IRQ_TC, s3c_ts_pen_updown, IRQF_SAMPLE_RANDOM, "ts_irq", s3c_ts_dev);
	request_irq(IRQ_ADC, s3c_ts_adc_irq, IRQF_SHARED | IRQF_SAMPLE_RANDOM, "adc_irq", s3c_ts_dev);

	wait_pen_down();
	//start_adc();
	init_timer(&ts_timer);
	ts_timer.function = ts_timer_func;
	add_timer(&ts_timer);

	return 0;
}

static void s3c_ts_exit(void)
{
	iounmap(ts_regs);
	disable_irq(IRQ_ADC);
	disable_irq(IRQ_TC);

	input_unregister_device(s3c_ts_dev);
	input_free_device(s3c_ts_dev);

	free_irq(IRQ_TC, s3c_ts_dev);
	free_irq(IRQ_ADC, s3c_ts_dev);

	del_timer(&ts_timer);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_LICENSE("GPL");

