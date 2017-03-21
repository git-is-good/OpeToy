#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include "kern/pci.h"

// PCI initialization related
#define	E1000_VENID	0x8086
#define	E1000_DEVID	0x100e

// Transmit related registers
#define	ETHER_TCTL	0x400	// 32bits
#define TCTL_EN		(1<<1)	
#define	TCTL_PSP	(1<<3)
#define	TCTL_CT_ETH	(0x10<<4)
#define	TCTL_COLD_FULL	(0x40<<12)

#define	ETHER_TIPG	0x410	//32bits
#define	TIPG_IPGT_IEEE	(10<<0)
#define	TIPG_IPGR1_IEEE	(8<<10)
#define	TIPG_IPGR2_IEEE	(6<<20)

#define	ETHER_AIFS	0x458
#define	ETHER_TDBAL	0x3800
#define	ETHER_TDBAH	0x3804
#define	ETHER_TDLEN	0x3808
#define	ETHER_TDH	0x3810
#define	ETHER_TDT	0x3818
#define	ETHER_TIDV	0x3820

// transmit descriptor
#define TDESC_ALIGN	16

#define	TDESC_CMD_DEXT	(1<<5)	// unset to use legacy mode
#define	TDESC_CMD_RS	(1<<3)	// let the card show feedback of whether a packet has been transmitted
#define	TDESC_CMD_EOP	(1<<0)	// indicate it is the last descriptor of a packet

#define	TDESC_STAT_DD	(1<<0)	// transmit done ?
#define	TRANS_BUFSZ	1518

struct Tdesc{
	uint64_t tdesc_buf;
	uint16_t tdesc_length;
	uint8_t  tdesc_cso;
	uint8_t  tdesc_cmd;
	uint8_t  tdesc_status;
	uint8_t  tdesc_css;
	uint16_t tdesc_special;
}__attribute__((aligned(TDESC_ALIGN)));

// Transmit descriptor config
#define	TRANS_NTDESC	8	//number of transmit descriptors, must be multiple of 8 to fit the 128 byte alignment of ETHER_TDLEE	

// Receive related registers
#define	ETHER_RCTL	0x100
#define	RCTL_EN		(1<<1)
#define	RCTL_LPE	(1<<5)	// set this bit will enable long packet reception, we don't use it
#define	RCTL_BSIZE_2048	(0<<16) // indicate the receive buffer size, put it just 0 for 2048bytes
#define	RCTL_SECRC	(1<<26)	// strip ethernet CRC before any processing

#define	ETHER_FCRTL	0x2160
#define	ETHER_FCRTH	0x2168
#define	ETHER_RDBAL	0x2800
#define	ETHER_RDBAH	0x2804
#define	ETHER_RDLEN	0x2808
#define	ETHER_RDH	0x2810
#define	ETHER_RDT	0x2818
#define	ETHER_RDTR	0x2820
#define	ETHER_RADV	0x282c
#define	ETHER_RSRPD	0x2c00

#define	ETHER_MTA	0x5200
#define	ETHER_MTA_LEN	0x200

#define	ETHER_RAL0	0x5400	// we only use the first one, there are 15 available
#define	ETHER_RAH0	0x5404
#define	RAH_VALID	(1<<31)	// set this to indicate this address is valid

// Receive descriptor
#define RDESC_ALIGN	16

#define	RDESC_STATUS_DD	(1<<0)

struct Rdesc{
	uint64_t rdesc_buf;
	uint16_t rdesc_length;
	uint16_t rdesc_checksum;
	uint8_t  rdesc_status;
	uint8_t  rdesc_errors;
	uint16_t rdesc_special;
}__attribute__((aligned(RDESC_ALIGN)));

// number of receive descriptors config
#define	RECV_NRDESC	128
// receive mac config
#define	MAC_ADDR_L	0x12005452
#define	MAC_ADDR_H	0x5634
// this config corresponds to the buffer size setting in RCTL
#define	RECV_BUFSZ	2048

int e1000_attach(struct pci_func *pcif);
int e1000_transmit(void *buf_to_trans, size_t sz);
int e1000_receive(void *buf_to_recv, size_t sz);

#endif	// JOS_KERN_E1000_H
