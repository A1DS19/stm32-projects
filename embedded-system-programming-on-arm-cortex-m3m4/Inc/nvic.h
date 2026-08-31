#ifndef NVIC_H
#define NVIC_H

/* Lesson: interrupts & the NVIC — system exceptions vs IRQs, software
 * pending, priority width, preemption live, PRIGROUP, tie-break. */
void playing_with_nvic(void);

/* What PendSV does before the scheduler starts: scheduler.c owns the
 * strong PendSV_Handler and branches here while sched_started is 0. */
void nvic_pendsv_demo(void);

#endif
