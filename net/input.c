#include <inc/lib.h>
#include "ns.h"

extern union Nsipc nsipcbuf;

// as defined in kern/e1000.h
#define	RECV_BUFSZ	2048

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r;
	while (1){
		struct jif_pkt *jp = &nsipcbuf.pkt;
		while( (r = sys_ether_try_recv(jp->jp_data, RECV_BUFSZ)) < 0 ){
			if ( r != -E_NO_RECV ){
				panic("input: %e\n");
			}
			sys_yield();
		}
		jp->jp_len = r;
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_U|PTE_W);
		assert(sys_page_unmap(0, &nsipcbuf) == 0);
		if ( (r = sys_page_alloc(0, &nsipcbuf, PTE_U|PTE_W)) < 0 ){
			panic("input: %e\n", r);
		}
	}
}
