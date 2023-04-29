/*-
 ****************************************************************************
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: sched.c
 *      Author: Grzegorz Milos
 *     Changes: Robert Kaiser
 *
 *        Date: Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: simple scheduler for Mini-Os
 *
 * The scheduler is non-preemptive (cooperative), and schedules according
 * to Round Robin algorithm.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <bmk-core/amd64/asm.h>

ENTRY(bmk_cpu_sched_bouncer)
	/* movq %rsp, %rax */
	/* movq $1, %rdi */
	/* movq $0, %rsi */
	/* movq $8, %rdx */
	/* movq $1, %rax */
	/* syscall */

	popq %rdi
	popq %rbx
	pushq $0			/* correct stack alignment for SSE */
	/* NIRCHG */
	/* pushq $0 */
	xorq %rbp,%rbp
	/* NIRCHG */

	/* movq %rbx, %rax      /* Move value in rbx to rax */
	/* movq $1, %rdi        /* Use file descriptor 1 for stdout */
	/* movq $0, %rsi        /* Buffer to write from (empty in this case) */
	/* movq $8, %rdx        /* Number of bytes to write (assuming 64-bit value) */
	/* movq $1, %rax        /* Syscall number for write */
	/* syscall              /* Call the write syscall to print value in rax */

	call *%rbx
	/* 3:	jmp 3b */
	call bmk_sched_exit
END(bmk_cpu_sched_bouncer)

ENTRY(bmk_cpu_sched_switch)
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq $1f                       /* save EIP */
	movq %rsp, (%rdi)               /* save ESP */
	movq (%rdx), %rsp               /* restore ESP */
	testq %rsi,%rsi
	jz 2f                           /* RSI = 0 */
	call (%rsi)                     /* callback function */
2:
	/* NIRCHG */
	/* 3:	jmp 3b */
	ret                             /* restore EIP */
1:
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret
END(bmk_cpu_sched_switch)
