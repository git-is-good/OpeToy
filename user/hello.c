// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
//	char hello[] = "this is a hello world message from far away\n";
//	sys_ether_try_send(hello, strlen(hello));
//	char rec[256];
//	sys_ether_try_recv(rec, 256);
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
}
