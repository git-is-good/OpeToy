// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "getsum", "sum from start to end", mon_getsum }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	unsigned ebp_value;
	__asm__ __volatile__(
	"movl %%ebp, %0"
	:"=q"(ebp_value)
	::);

	cprintf("Stack backtrace:\n");
	while ( ebp_value ){
		unsigned eip_value = *((unsigned*)ebp_value + 1);
		cprintf("  ebp %08x  eip %08x  args", ebp_value, eip_value);
		for ( int i = 0; i < 5; i++ ){
			cprintf(" %08x", *((unsigned*)ebp_value + 2+i) );
		}
		cprintf("\n");
		struct Eipdebuginfo info;
		debuginfo_eip(eip_value, &info);
		cprintf("         %s:%d: ", info.eip_file, info.eip_line);
		for( int i = 0; i < info.eip_fn_namelen; i++ ){
			cputchar(info.eip_fn_name[i]);
		}
		cprintf("+%d\n", eip_value - info.eip_fn_addr);
		ebp_value = *(unsigned*)ebp_value;
	}
	return 0;
}

static int mon_atoi(char *s){
	int n = 0;
	while ( *s ){
		n = 10*n + (*s++ - '0');
	}
	return n;
}

int mon_getsum(int argc, char **argv, struct Trapframe* tf){
	if ( argc != 3 ){
		cprintf("usage: start end\n");
		return 0;
	}
	int start = mon_atoi(argv[1]);
	int end = mon_atoi(argv[2]);
	int sum = 0;
	while (start < end){
		sum += start++;
	}
	cprintf("sum from %d to %d: %d\n", start, end, sum);
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
