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


/* ģ��������� */
static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
	/* �ο�LDD3 */
	unsigned char *type;
	struct iphdr *ih;
	__be32 *saddr, *daddr, tmp;
	unsigned char tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;
	struct sk_buff *rx_skb;
	// ��Ӳ������/��������
	/* �Ե�"Դ/Ŀ��"��mac��ַ */
	ethhdr = (struct ethhdr *)skb->data;		//��ȡ��MAC�ṹ��ָ��
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);		//Ŀ���͸���ʱ
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);	//Դ�͸�Ŀ��
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);		//��ʱ�͸�Դ

	/* �Ե�"Դ/Ŀ��"��ip��ַ */
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));		//��ȡ��ip�ṹ��ָ�룬ipͷ��MAC֮��
	saddr = &ih->saddr;		//Դ
	daddr = &ih->daddr;		//Ŀ��

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;
	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);	//��ȡtype
	 
	// �޸�����, ԭ��0x8��ʾping
	*type = 0;		/* 0��ʾreply */
	ih->check = 0;	/* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
	// ����һ��sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2); /* Ԥ�������ֽ� */ 
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);//���ڱ��д�������ע��2

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	// �ύsk_buff
	netif_rx(rx_skb);
}

static netdev_tx_t net_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	static int count = 0;
	printk(KERN_INFO "net_send_packet: %d\n", ++count);

	/* ����ͳ����Ϣ */
	dev->stats.tx_packets ++;
	dev->stats.tx_bytes += skb->len;


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
	/* 2.1 ��ʼ��netde_ops */
	vnet_dev->netdev_ops = &net_ops;
	/* 2.2 ����MAC��ַ */
	vnet_dev->dev_addr[0] = 0x01;
	vnet_dev->dev_addr[1] = 0x02;
	vnet_dev->dev_addr[2] = 0x03;
	vnet_dev->dev_addr[3] = 0x04;
	vnet_dev->dev_addr[4] = 0x05;
	vnet_dev->dev_addr[5] = 0x06;

	vnet_dev->flags |= IFF_NOARP;
	vnet_dev->features |= NETIF_F_NO_CSUM; 
	
	/* 3. ע�� */
	register_netdev(vnet_dev);

	return 0;
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
