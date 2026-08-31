#ifndef SCHEDULER_H
#define SCHEDULER_H

/* Lesson: the task scheduler — four tasks on private stacks, a 1 ms
 * SysTick, PendSV doing the context switch, blocking delays and an idle
 * task. The course's capstone. Never returns. */

enum SchedMode {
    SCHED_SPIN_DELAYS,     /* slides' first build: tasks burn CPU to wait — idle never runs */
    SCHED_BLOCKING_DELAYS, /* final build: tasks block in task_delay() — idle runs, timing exact */
};

void playing_with_scheduler(enum SchedMode mode);

#endif
