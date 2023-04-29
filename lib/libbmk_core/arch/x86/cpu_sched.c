/*
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

#include <bmk-core/core.h>
#include <bmk-core/sched.h>

// NIRCHG
#include <bmk-core/printf.h>
#include <bmk-core/mainthread.h>

extern int test_memory;

static void
stack_push(void **stackp, unsigned long value)
{
	unsigned long *stack = *stackp;

	stack--;
	*stack = value;
	*stackp = stack;
}

void temp(void *ptr) {
	printf("In temp -> replacement of f -> bmk_mainthread\n");
	// while (1);
}

void
bmk_cpu_sched_create(struct bmk_thread *thread, struct bmk_tcb *tcb,
	void (*f)(void *), void *arg,
	void *stack_base, unsigned long stack_size)
{
	// NIRCHG
	// printf("At the start of bmk_cpu_sched_create\n");

	// printf("test_memory in bmk_cpu_sched_create = %p\n", &test_memory);
	// printf("test_memory from bmk_cpu_sched_create = %x\n", test_memory);

	// test_main(0);

	// NIRCHG
	// if (f == bmk_mainthread) {
		// printf("f is same as bmk_mainthread\n");
		// printf("In bmk_cpu_sched_create, f -> %p\n", f);
		// printf("In bmk_cpu_sched_create, temp -> %p\n", temp);
	// }
	// printf("In bmk_cpu_sched_create, f -> %p\n", f);

	void *stack_top = (char *)stack_base + stack_size;

	// printf("stack_base = %p\n", stack_base);
	// printf("stack_size = %x\n", stack_size);

	/* Save pointer to the thread on the stack, used by current macro */
	*(unsigned long *)stack_base = (unsigned long)thread;

	// printf("stack_top = %p\n", stack_top);

	stack_push(&stack_top, 0);

	// printf("stack_top = %p\n", stack_top);

	/* these values are used by bmk_cpu_sched_bouncer() */
	// stack_push(&stack_top, (unsigned long)f);

	// NIRCHG
	// if (f == bmk_mainthread) {
		// stack_push(&stack_top, (unsigned long)temp);
		// stack_push(&stack_top, (unsigned long)f);
		// stack_push(&stack_top, (unsigned long)0);
		// stack_push(&stack_top, (unsigned long)arg);
	// }
	// else {
		stack_push(&stack_top, (unsigned long)f);
		stack_push(&stack_top, (unsigned long)arg);
	// }

	// printf("stack_top = %p\n", stack_top);

	stack_push(&stack_top, (unsigned long)bmk_cpu_sched_bouncer);

	tcb->btcb_sp = (unsigned long)stack_top;
	// NIRCHG
	// tcb->btcb_ip = (unsigned long)bmk_cpu_sched_bouncer;

	// printf("stack_top = %p\n", stack_top);
}
