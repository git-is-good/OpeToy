// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	// LAB 4: Your code here.
	if ( !(uvpd[PDX(addr)] & PTE_P) || 
		(!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW)) ){
		panic("pgfault: real page fault\n");
	}
	//if ( !(err & FEC_WR) || !(pentry & PTE_COW) ){
	//	panic("pgfault: real page fault\n");
	//}


	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	uint32_t envid = sys_getenvid();
	if ( (r = sys_page_alloc(envid, (void*)PFTEMP, PTE_W)) < 0 ){
		panic("pgfault: %e\n", r);
	}
	memcpy(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	r = sys_page_map(envid, (void*)PFTEMP, envid, (void*)ROUNDDOWN(addr, PGSIZE), PTE_W);
	assert(r == 0);

	// LAB 4: Your code here.

	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	
	if ( !(uvpd[pn >> 10] & PTE_P) ) return 0;
	uint32_t pentry = uvpt[pn];
	if ( !(pentry & PTE_P) ) return 0;
	assert( pentry & PTE_U );

	if ( pentry & PTE_SHARE ){
		r = sys_page_map(0, (void*)(pn*PGSIZE), envid, (void*)(pn*PGSIZE), pentry & PTE_SYSCALL);
		if ( r < 0 ) goto fail;
	}else if ( (pentry & PTE_W) || (pentry & PTE_COW) ){
		r = sys_page_map(0, (void*)(pn*PGSIZE), envid, (void*)(pn*PGSIZE), PTE_COW);
		if ( r < 0 ) goto fail;//panic("duppage: %e\n", r);
		r = sys_page_map(0, (void*)(pn*PGSIZE), 0, (void*)(pn*PGSIZE), PTE_COW);
		if ( r < 0 ) goto fail;//panic("duppage: %e\n", r);
	}else{
		r = sys_page_map(0, (void*)(pn*PGSIZE), envid, (void*)(pn*PGSIZE), PTE_U);
		if ( r < 0 ) goto fail;//panic("duppage: %e\n", r);
	}

	// LAB 4: Your code here.
//	panic("duppage not implemented");
	return 0;
fail:
	panic("duppage: %e", r);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t childid = sys_exofork();
	if ( childid < 0 ){
		panic("fork: %e\n", childid);
	}
	if ( childid > 0 ){
		int r = sys_page_alloc(childid, (void*)(UXSTACKTOP - PGSIZE), PTE_W);
		assert(r == 0);
		for ( uint32_t pn = USTABDATA/PGSIZE; pn < USTACKTOP/PGSIZE;){
			if ( !(uvpd[pn >> 10] & PTE_P) ){
				pn = ROUNDDOWN( pn + (1 << 10), 1 << 10 );
				continue;
			}
			duppage(childid, pn++);
		}
		extern void* _pgfault_upcall();
		sys_env_set_pgfault_upcall(childid, _pgfault_upcall);
		assert( PGSIZE - (uint32_t)&thisenv % PGSIZE >= sizeof(uint32_t) );
		sys_page_alloc(0, (void*)PFTEMP, PTE_W);
		memcpy((void*)PFTEMP, (void*)ROUNDDOWN(&thisenv, PGSIZE), PGSIZE);
		*(struct Env**)(PFTEMP + (uint32_t)&thisenv % PGSIZE) = (struct Env*)envs + ENVX(childid); 
		sys_page_map(0, (void*)PFTEMP, childid, (void*)ROUNDDOWN(&thisenv, PGSIZE), PTE_W);
		sys_env_set_status(childid, ENV_RUNNABLE);
	}
	return childid;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
