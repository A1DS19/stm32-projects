#ifndef SVC_H
#define SVC_H

/* Lesson: SVC, the system-call exception — the request number dug out
 * of the opcode behind the stacked pc, results returned through the
 * stacked r0, and the one rule: never SVC inside the SVC handler. */
void playing_with_svc(void);

#endif
