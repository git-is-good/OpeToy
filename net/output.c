#include "inc/lib.h"
#include "inc/types.h"
#include "inc/mmu.h"
#include "ns.h"

// defined in kern/e1000.h
#define	TRANS_BUFSZ	1518 // size limit of a packet

#define	debug	0
extern union Nsipc nsipcbuf;

// -- This function will be an endless loop
// -- The core network server fork a child process, and let the child run
//    this helper function endlessly.
// -- ns_envid is the core network server for which this function serves
void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	
	while(1){
		envid_t from_envid;
		int32_t serv_id = ipc_recv(&from_envid, &nsipcbuf, NULL);
		if ( from_envid != ns_envid ){
			if ( debug ){
				cprintf("output: not sent by my core server!\n");
			}
			continue;
		}
		if ( serv_id != NSREQ_OUTPUT ){
			if ( debug ){
				cprintf("output: not NSREQ_OUTPUT!\n");
			}
			continue;
		}
		struct jif_pkt *jp = &nsipcbuf.pkt;
		int r;
		assert( jp->jp_len <= PGSIZE - sizeof(int) );
		if ( debug ){
			cprintf("output: get packet from core server: size: %d\n", jp->jp_len);
		}
		for ( int i = 0; i < jp->jp_len; i += TRANS_BUFSZ ){
			int sz = MIN(TRANS_BUFSZ, jp->jp_len - i);
			while ( (r = sys_ether_try_send(jp->jp_data + i, sz)) < 0 ){
				if ( r != -E_FULL_BUF ){
					panic("output: %e\n", r);
				}
				sys_yield();
			}
		}
	}
}
