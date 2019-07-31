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

.section .user_mapped.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

/* exceptions handler */
TRAPHANDLER_NOEC(t_divide_handler, T_DIVIDE) /* 0 */
TRAPHANDLER_NOEC(t_debug_handler, T_DEBUG) /* 1 */
TRAPHANDLER_NOEC(t_nmi_handler, T_NMI) /* 2 */
TRAPHANDLER_NOEC(t_brkpt_handler, T_BRKPT) /* 3 */
TRAPHANDLER_NOEC(t_oflow_handler, T_OFLOW) /* 4 */
TRAPHANDLER_NOEC(t_bound_handler, T_BOUND) /* 5 */
TRAPHANDLER_NOEC(t_illop_handler, T_ILLOP) /* 6 */
TRAPHANDLER_NOEC(t_device_handler, T_DEVICE) /* 7 */
TRAPHANDLER(t_dblflt_handler,T_DBLFLT) /* 8 */
                                /* reserved 9 */
TRAPHANDLER(t_tss_handler,T_TSS) /* 10 */
TRAPHANDLER(t_segnp_handler,T_SEGNP) /* 11 */
TRAPHANDLER(t_stack_handler,T_STACK) /* 12 */
TRAPHANDLER(t_gpflt_handler,T_GPFLT) /* 13 */
TRAPHANDLER(t_pgflt_handler,T_PGFLT) /* 14 */
                            /* reserved 15 */
TRAPHANDLER_NOEC(t_fperr_handler, T_FPERR) /* 16 */
TRAPHANDLER(t_align_handler, T_ALIGN) /* 17 */
TRAPHANDLER_NOEC(t_mchk_handler, T_MCHK) /* 18 */
TRAPHANDLER_NOEC(t_simderr_handler, T_SIMDERR) /* 19 */
TRAPHANDLER_NOEC(t_syscall_handler, T_SYSCALL) /* 48 */


/* hardware interrupts handler */
TRAPHANDLER_NOEC(irq_timer_handler, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER_NOEC(irq_kbd_handler, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(irq_2_handler, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(irq_3_handler, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(irq_serial_handler, IRQ_OFFSET + IRQ_SERIAL)
TRAPHANDLER_NOEC(irq_5_handler, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(irq_6_handler, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(irq_spurious_handler, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER_NOEC(irq_8_handler, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(irq_9_handler, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(irq_10_handler, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(irq_11_handler, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(irq_12_handler, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(irq_13_handler, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(irq_ide_handler, IRQ_OFFSET + IRQ_IDE)
TRAPHANDLER_NOEC(irq_15_handler, IRQ_OFFSET + 15)

/*
.globl sysenter_handler;
.type sysenter_handler, @function;
.align 2;
sysenter_handler:
    pushl %edi
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %eax
    call syscall
    movl %ebp, %ecx
    movl %esi, %edx
    sysexit
 */

/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD, %eax
	movw %ax,%ds
	movw %ax,%es
	pushl %esp
	call switch_and_trap
