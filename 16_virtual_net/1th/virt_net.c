/* �ο�cs89x0.c */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static netdev_tx_t
net_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	static int count = 0;
	printk(KERN_INFO "net_send_packet: %d\n", ++count);

	return NETDEV_TX_OK;
}

/* ʵ��һ��net_device_ops�Ľṹ�� */
static const struct net_device_ops net_ops = {
	.ndo_start_xmit 	= net_send_packet,
};

static struct net_device *vnet_dev;

static int __init vnet_init(void)
{	
	/* 1. ����һ��net_device */
	vnet_dev = alloc_netdev(0, "vnet%d", ether_setup);		//alloc_etherdev

	/* 2. ��ʼ�� */
	//vnet_dev->netdev_ops = &net_ops;
	
	/* 3. ע�� */
	register_netdev(vnet_dev);
}

static void __exit vnet_exit(void)
{
	/* 1. ע�� */
	unregister_netdev(vnet_dev);

	/* 2. �ͷ� */
	free_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
