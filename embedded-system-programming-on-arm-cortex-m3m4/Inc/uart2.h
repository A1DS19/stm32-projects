#ifndef UART2_H
#define UART2_H

/* USART2 on PA2/PA3 — the Nucleo's ST-LINK virtual COM port. 115200 8N1.
 * uart2_init() also makes printf work: syscall.c routes _write -> __io_putchar,
 * and uart2.c provides the strong __io_putchar. */
void uart2_init(void);

/* Non-blocking: one received byte (0..255), or -1 if none is waiting. */
int uart2_poll(void);

/* Blocking: waits for one received byte and returns it (0..255). */
int uart2_getc(void);

#endif
