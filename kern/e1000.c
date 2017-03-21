#include <inc/assert.h>
#include <inc/error.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/mmu.h>
#include <kern/pmap.h>
#include <kern/e1000.h>

// LAB 6: Your driver code here

// beginning of memory mapped i/o for e1000
static volatile char *e1000_addr;

// transmit descriptor table
static volatile struct Tdesc tdescs[TRANS_NTDESC];

// receive descriptor table
static volatile struct Rdesc rdescs[RECV_NRDESC];

// one pages holds 2 buffer
// the buffer size for e1000 to use is 1518, however, this buffer must be contiguous
// thus we allocate for every buffer 2000 byte, one page 2 such buffer
#define	TNBUF_PERPG	2 //(PGSIZE/TRANS_BUFSZ)
#define TBUF_ALLOC	(PGSIZE/2)
#define	RNBUF_PERPG	2 //(PGSIZE/RECV_BUFSZ)
#define	RBUF_ALLOC	(PGSIZE/2)

static void init_trans(){
	// allocate space for transbuf
	assert(TRANS_NTDESC % TNBUF_PERPG == 0); // number of buffer is even
	for ( int i = 0; i < TRANS_NTDESC / TNBUF_PERPG; i++ ){
		struct PageInfo *p = page_alloc(0);
		p->pp_ref++;		
		if (!p){
			panic("init_trans: %e\n", E_NO_MEM);
		}
		for ( int j = 0; j < TNBUF_PERPG; j++ ){
			tdescs[i * TNBUF_PERPG + j].tdesc_buf = (uint32_t)page2pa(p) + j * TBUF_ALLOC;
		}
	}

	// set lagecy mode already done( because it's 0 )
	// set command RS bit to get feedback
	// ** Be careful! the packet will be transmitted only if EOP is set
	// ** otherwise the card will wait for subsequent descriptor without
	// ** transmitting anything...
	// at the beginning, status DD bit should be set
	for ( int i = 0; i < TRANS_NTDESC; i++ ){
		tdescs[i].tdesc_cmd = TDESC_CMD_RS | TDESC_CMD_EOP;
	 	tdescs[i].tdesc_status = TDESC_STAT_DD;
	}

	// TDBAL/TDBAH
	// Be careful! every address passed to the network card must be physical address
	*(uint32_t*)(e1000_addr + ETHER_TDBAL) = PADDR((void*)tdescs);
	*(uint32_t*)(e1000_addr + ETHER_TDBAH) = 0;

	// init TDLEN
	static_assert( sizeof(struct Tdesc) * TRANS_NTDESC % 128 == 0 ); // hardware requirement
	*(uint64_t*)(e1000_addr + ETHER_TDLEN) = sizeof(struct Tdesc) * TRANS_NTDESC; 

	// init TDH/TDT
	*(uint64_t*)(e1000_addr + ETHER_TDH) = 0;
	*(uint64_t*)(e1000_addr + ETHER_TDT) = 0;

	// init TCTL
	*(uint32_t*)(e1000_addr + ETHER_TCTL) = TCTL_EN | TCTL_PSP | TCTL_CT_ETH | TCTL_COLD_FULL;

	// init TIPG
	*(uint32_t*)(e1000_addr + ETHER_TIPG) = TIPG_IPGT_IEEE | TIPG_IPGR1_IEEE | TIPG_IPGR2_IEEE;
}

static void init_recv(){
	// init receive buffers
	assert(RECV_NRDESC % RNBUF_PERPG == 0); // number of buffer is even
	for ( int i = 0; i < RECV_NRDESC / RNBUF_PERPG; i++ ){
		struct PageInfo *p = page_alloc(0);
		p->pp_ref++;		
		if (!p){
			panic("init_recv: %e\n", E_NO_MEM);
		}
		for ( int j = 0; j < RNBUF_PERPG; j++ ){
			rdescs[i * RNBUF_PERPG + j].rdesc_buf = (uint32_t)page2pa(p) + j * RBUF_ALLOC;
		}
	}
	
	// set RAL[0] and RAH[0]
	*(uint32_t*)(e1000_addr + ETHER_RAL0) = MAC_ADDR_L;
	*(uint32_t*)(e1000_addr + ETHER_RAH0) = MAC_ADDR_H | RAH_VALID;

	// set MTA to 0
	memset((void*)(e1000_addr + ETHER_MTA), 0, ETHER_MTA_LEN);

	// set RDBAL/RDBAH
	*(uint32_t*)(e1000_addr + ETHER_RDBAL) = PADDR((void*)rdescs);
	*(uint32_t*)(e1000_addr + ETHER_RDBAH) = 0;

	// set RDH/RDT
	// hardware stops receiving when RDH == RDT
	*(uint32_t*)(e1000_addr + ETHER_RDH) = 1;
	*(uint32_t*)(e1000_addr + ETHER_RDT) = 0;

	// set RDLEN
	static_assert( sizeof(struct Rdesc) * RECV_NRDESC % 128 == 0 ); // hardware requirement
	*(uint32_t*)(e1000_addr + ETHER_RDLEN) = sizeof(struct Rdesc) * RECV_NRDESC;

	// set RCTL
	*(uint32_t*)(e1000_addr + ETHER_RCTL) = RCTL_EN | RCTL_SECRC;
}

int e1000_transmit(void *buf_to_trans, size_t sz){
	if ( sz > TRANS_BUFSZ ){
		return -E_INVAL;
	}
	uint32_t ind = *(uint32_t*)(e1000_addr + ETHER_TDT);
	if ( !(tdescs[ind].tdesc_status & TDESC_STAT_DD) ){
		return -E_FULL_BUF;
	}
	memcpy((void*)KADDR(tdescs[ind].tdesc_buf), buf_to_trans, sz);
	tdescs[ind].tdesc_length = sz;
	tdescs[ind].tdesc_status &= ~TDESC_STAT_DD;
	*(uint32_t*)(e1000_addr + ETHER_TDT) = (ind + 1) % TRANS_NTDESC;
	return 0;
}

int e1000_receive(void *buf_to_recv, size_t sz){
	if ( sz > RECV_BUFSZ ){
		return -E_INVAL;
	}
	uint32_t ind = (*(uint32_t*)(e1000_addr + ETHER_RDT) + 1) % RECV_NRDESC;
	if ( !(rdescs[ind].rdesc_status & RDESC_STATUS_DD) ){
		return -E_NO_RECV;
	}
	// the actual size is the minimum of sz and rdesc_length
	size_t acsz = MIN(sz, rdescs[ind].rdesc_length);
	memcpy(buf_to_recv, (void*)KADDR(rdescs[ind].rdesc_buf), acsz);
	rdescs[ind].rdesc_status &= ~RDESC_STATUS_DD;
	*(uint32_t*)(e1000_addr + ETHER_RDT) = ind;
	return acsz;
}

int e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	assert(pcif->reg_base[0] % PGSIZE == 0);
	e1000_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	init_trans();
	init_recv();
	return 0;
}

