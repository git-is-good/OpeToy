/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

TRAPHANDLER_NOEC(idt_entry0, T_DIVIDE)
TRAPHANDLER_NOEC(idt_entry1, T_DEBUG)
TRAPHANDLER_NOEC(idt_entry2, T_NMI)
TRAPHANDLER_NOEC(idt_entry3, T_BRKPT)
TRAPHANDLER_NOEC(idt_entry4, T_OFLOW)
TRAPHANDLER_NOEC(idt_entry5, T_BOUND)
TRAPHANDLER_NOEC(idt_entry6, T_ILLOP)
TRAPHANDLER_NOEC(idt_entry7, T_DEVICE)

// entry 8 - 14 will generate error code
TRAPHANDLER(idt_entry8, T_DBLFLT)
// no entry 9
TRAPHANDLER(idt_entry10, T_TSS)
TRAPHANDLER(idt_entry11, T_SEGNP)
TRAPHANDLER(idt_entry12, T_STACK)
TRAPHANDLER(idt_entry13, T_GPFLT)
TRAPHANDLER(idt_entry14, T_PGFLT)

// no entry 15
TRAPHANDLER_NOEC(idt_entry16, T_FPERR)
TRAPHANDLER_NOEC(idt_entry17, T_ALIGN)
TRAPHANDLER_NOEC(idt_entry18, T_MCHK)
TRAPHANDLER_NOEC(idt_entry19, T_SIMDERR)

TRAPHANDLER_NOEC(idt_entry0x30, T_SYSCALL)

TRAPHANDLER_NOEC(irq_entry0, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER_NOEC(irq_entry1, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(irq_entry2, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(irq_entry3, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(irq_entry4, IRQ_OFFSET + IRQ_SERIAL)
TRAPHANDLER_NOEC(irq_entry5, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(irq_entry6, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(irq_entry7, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER_NOEC(irq_entry8, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(irq_entry9, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(irq_entry10, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(irq_entry11, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(irq_entry12, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(irq_entry13, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(irq_entry14, IRQ_OFFSET + IRQ_IDE)
TRAPHANDLER_NOEC(irq_entry15, IRQ_OFFSET + 15)

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
_alltraps:
	pushw	$0
	pushw	%ds
	pushw	$0
	pushw	%es
	pushal
	movw	$GD_KD,	%ax
	movw	%ax,	%ds
	movw	%ax,	%es
	pushl	%esp
	call	trap
	// should never return



/*
 * Lab 3: Your code here for _alltraps
 */

