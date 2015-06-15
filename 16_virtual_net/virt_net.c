/* 参考cs89x0.c */

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


/* 模拟接收数据 */
static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
	/* 参考LDD3 */
	unsigned char *type;
	struct iphdr *ih;
	__be32 *saddr, *daddr, tmp;
	unsigned char tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;
	struct sk_buff *rx_skb;
	// 从硬件读出/保存数据
	/* 对调"源/目的"的mac地址 */
	ethhdr = (struct ethhdr *)skb->data;		//提取出MAC结构体指针
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);		//目的送给临时
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);	//源送给目的
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);		//临时送给源

	/* 对调"源/目的"的ip地址 */
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));		//提取出ip结构体指针，ip头在MAC之后
	saddr = &ih->saddr;		//源
	daddr = &ih->daddr;		//目的

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;
	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);	//提取type
	 
	// 修改类型, 原来0x8表示ping
	*type = 0;		/* 0表示reply */
	ih->check = 0;	/* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
	// 构造一个sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2); /* 预留两个字节 */ 
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);//关于本行代码详解见注释2

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	// 提交sk_buff
	netif_rx(rx_skb);
}

static netdev_tx_t net_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	static int count = 0;
	printk(KERN_INFO "net_send_packet: %d\n", ++count);

	/* 更新统计信息 */
	dev->stats.tx_packets ++;
	dev->stats.tx_bytes += skb->len;


	return NETDEV_TX_OK;
}

/* 实现一个net_device_ops的结构体 */
static const struct net_device_ops net_ops = {
	.ndo_start_xmit 	= net_send_packet,
};

static struct net_device *vnet_dev;

static int __init vnet_init(void)
{	
	/* 1. 分配一个net_device */
	vnet_dev = alloc_netdev(0, "vnet%d", ether_setup);		//alloc_etherdev

	/* 2. 初始化 */
	/* 2.1 初始化netde_ops */
	vnet_dev->netdev_ops = &net_ops;
	/* 2.2 设置MAC地址 */
	vnet_dev->dev_addr[0] = 0x01;
	vnet_dev->dev_addr[1] = 0x02;
	vnet_dev->dev_addr[2] = 0x03;
	vnet_dev->dev_addr[3] = 0x04;
	vnet_dev->dev_addr[4] = 0x05;
	vnet_dev->dev_addr[5] = 0x06;

	vnet_dev->flags |= IFF_NOARP;
	vnet_dev->features |= NETIF_F_NO_CSUM; 
	
	/* 3. 注册 */
	register_netdev(vnet_dev);

	return 0;
}

static void __exit vnet_exit(void)
{
	/* 1. 注消 */
	unregister_netdev(vnet_dev);

	/* 2. 释放 */
	free_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
