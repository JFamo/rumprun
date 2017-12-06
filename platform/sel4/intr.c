/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/*-
 * Copyright (c) 2014 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sel4/kernel.h>
#include <sel4/helpers.h>
#include <sel4/sel4.h>
#include <bmk-core/core.h>
#include <bmk-core/memalloc.h>
#include <bmk-core/printf.h>
#include <bmk-core/queue.h>
#include <bmk-core/sched.h>

#include <bmk-rumpuser/rumpuser.h>

#define INTR_LEVELS (BMK_MAXINTR+1)
#define INTR_ROUTED BMK_MAXINTR

struct intrhand {
    int (*ih_fun)(void *);
    void *ih_arg;

    SLIST_ENTRY(intrhand) ih_entries;
};

SLIST_HEAD(isr_ihead, intrhand);
static struct isr_ihead isr_ih[INTR_LEVELS];

static volatile unsigned int isr_todo;
static unsigned int isr_lowest = sizeof(isr_todo) * 8;

static struct bmk_thread *isr_thread;


/* thread context we use to deliver interrupts to the rump kernel */
static void
doisr(void *arg)
{
    int i, totwork = 0;

    rumpuser__hyp.hyp_schedule();
    rumpuser__hyp.hyp_lwproc_newlwp(0);
    rumpuser__hyp.hyp_unschedule();

    bmk_platform_splhigh();
    for (;;) {
        unsigned int isrcopy;
        int nlocks = 1;

        isrcopy = isr_todo;
        isr_todo = 0;
        bmk_platform_splx(0);

        totwork |= isrcopy;

        rumpkern_sched(nlocks, NULL);
        for (i = isr_lowest; isrcopy; i++) {
            struct intrhand *ih;

            bmk_assert(i < sizeof(isrcopy) * 8);
            if ((isrcopy & (BIT(i))) == 0) {
                continue;
            }
            isrcopy &= ~(BIT(i));

            SLIST_FOREACH(ih, &isr_ih[i], ih_entries) {
                ih->ih_fun(ih->ih_arg);
            }
            /* Ack seL4 interrupt now that it has been handled */
            if (is_hw_pci_config(&env.custom_simple)) {
                if (!config_set(CONFIG_USE_MSI_ETH)) {
                    int error = seL4_IRQHandler_Ack(env.caps[i]);
                    ZF_LOGF_IFERR(error, "seL4_IRQHandler_Ack failed");
                }
            } else {
                env.custom_simple.ethernet_intr_config.eth_irq_acknowledge();
            }
        }
        rumpkern_unsched(&nlocks, NULL);

        bmk_platform_splhigh();
        if (isr_todo) {
            continue;
        }

        /* no interrupts left. block until the next one. */
        bmk_sched_blockprepare();

        bmk_platform_splx(0);
        bmk_sched_block();
        totwork = 0;
        bmk_platform_splhigh();
    }
}

void
bmk_isr_rumpkernel(int (*func)(void *), void *arg, int intr)
{
    struct intrhand *ih;
    if (intr > sizeof(isr_todo) * 8 || intr > BMK_MAXINTR) {
        bmk_platform_halt("bmk_isr_rumpkernel: intr");
    }

    ih = bmk_xmalloc_bmk(sizeof(*ih));
    if (!ih) {
        bmk_platform_halt("bmk_isr_rumpkernel: xmalloc");
    }

    ih->ih_fun = func;
    ih->ih_arg = arg;

    SLIST_INSERT_HEAD(&isr_ih[intr], ih, ih_entries);
    if ((unsigned)intr < isr_lowest) {
        isr_lowest = intr;
    }
}

void
isr(int which)
{
    /* schedule the interrupt handler */
    isr_todo |= which;
    bmk_sched_wake(isr_thread);
}

void
intr_init(void)
{
    int i;

    for (i = 0; i < INTR_LEVELS; i++) {
        SLIST_INIT(&isr_ih[i]);
    }

    isr_thread = bmk_sched_create("isrthr", NULL, 0, doisr, NULL, NULL, 0);
    if (!isr_thread) {
        bmk_platform_halt("intr_init");
    }
}
