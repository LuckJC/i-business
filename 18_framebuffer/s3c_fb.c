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
#include <linux/fb.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct fb_info s3c_fb_info;

static int s3c_fb_init(void)
{
	/* 可变的屏幕参数 */
	s3c_fb_info.var.xres = 480;
	s3c_fb_info.var.yres = 272;
	s3c_fb_info.var.xres_virtual = 480;
	s3c_fb_info.var.yres_virtual = 270;
	s3c_fb_info.var.xoffset = 0;
	s3c_fb_info.var.yoffset = 0;
	s3c_fb_info.var.bits_per_pixel = 0;
	/* 固定的屏幕参数 */
	s3c_fb_info.fix = ;

	/* 其它 */
	fbops
	screen_base
	screen_size
	pseudo_palette
	
	return 0;
}

static void s3c_fb_exit(void)
{
	;
}

module_init(s3c_fb_init);
module_exit(s3c_fb_exit);

MODULE_LICENSE("GPL");