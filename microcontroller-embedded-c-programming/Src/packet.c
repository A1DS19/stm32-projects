/* Lesson: decode a 32-bit network packet — type it in, extract the fields.
 *
 * First lesson with UART INPUT: you type a hex value into `make serial`,
 * the firmware reads it character by character (uart2_getc), turns the
 * text into a number, and pulls the packet fields out of it — twice:
 *
 *   1) by hand, with shift + mask. The one formula to remember:
 *          field = (value >> position) & ((1 << width) - 1)
 *      ">> position" slides the field down to bit 0;
 *      "(1 << width) - 1" builds a mask of `width` ones (e.g. 12 ones);
 *      "&" keeps only those bits.
 *   2) with the bit-field union from the previous lesson — assign the
 *      whole value to .word once, then every field is just a named read.
 *
 * Both must agree bit for bit; the program checks that they do.
 *
 * The packet layout (32 bits total, bit 0 = rightmost):
 *
 *   bit 31    30..29     28..21    20..18   17..15   14..3     2      1..0
 *   [addr_mode][short_addr][long_addr][sensor ][battery][payload][status][crc]
 *        1         2           8         3        3       12       1      2
 */

#include "packet.h"
#include "uart2.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Same layout as the picture above. Remember from the bit-field lesson:
 * on this compiler the FIRST field takes the LOWEST bits. */
union packet {
  struct {
    uint32_t crc : 2;
    uint32_t status : 1;
    uint32_t payload : 12;
    uint32_t battery : 3;
    uint32_t sensor : 3;
    uint32_t long_addr : 8;
    uint32_t short_addr : 2;
    uint32_t addr_mode : 1;
  } f;
  uint32_t word;
};

/* Read one line of text: echo each key back (so you see what you type),
 * support backspace, finish on Enter. Enter arrives as '\r' from most
 * terminals. Returns the text without the line ending. */
static void read_line(char *buf, uint32_t size)
{
  uint32_t n = 0;
  for (;;) {
    int c = uart2_getc();
    if (c == '\r' || c == '\n') {
      printf("\r\n");
      buf[n] = '\0';
      return;
    }
    if (c == 0x08 || c == 0x7F) { /* backspace or delete key */
      if (n > 0) {
        n--;
        printf("\b \b"); /* step back, wipe the char, step back again */
      }
      continue;
    }
    if (n < size - 1) {
      buf[n++] = (char)c;
      putchar(c); /* echo */
    }
  }
}

void playing_with_packet(void)
{
  uart2_init();

  printf("\r\n== packet decode: type a 32-bit hex value ==\r\n");

  char line[16];
  for (;;) {
    printf("\r\npacket> 0x");
    read_line(line, sizeof(line));

    /* Text -> number, base 16. `end` is left pointing at the first
     * character strtoul could not use — if that is the start, nothing
     * was a hex digit at all. */
    char *end;
    uint32_t value = strtoul(line, &end, 16);
    if (end == line) {
      printf("not a hex number, try e.g. DEADBEEF\r\n");
      continue;
    }

    printf("value = 0x%08lX\r\n\r\n", (unsigned long)value);

    /* Way 1: shift + mask, field by field. */
    uint32_t crc = (value >> 0) & 0x3;
    uint32_t status = (value >> 2) & 0x1;
    uint32_t payload = (value >> 3) & 0xFFF;
    uint32_t battery = (value >> 15) & 0x7;
    uint32_t sensor = (value >> 18) & 0x7;
    uint32_t long_addr = (value >> 21) & 0xFF;
    uint32_t short_addr = (value >> 29) & 0x3;
    uint32_t addr_mode = (value >> 31) & 0x1;

    /* Way 2: one assignment, then the union's names do the slicing. */
    union packet p = {.word = value};

    printf("  field       bits    shift+mask   bit-field\r\n");
    printf("  crc         [1:0]   %10lu  %10lu\r\n",
           (unsigned long)crc, (unsigned long)p.f.crc);
    printf("  status      [2]     %10lu  %10lu\r\n",
           (unsigned long)status, (unsigned long)p.f.status);
    printf("  payload     [14:3]  %10lu  %10lu\r\n",
           (unsigned long)payload, (unsigned long)p.f.payload);
    printf("  battery     [17:15] %10lu  %10lu\r\n",
           (unsigned long)battery, (unsigned long)p.f.battery);
    printf("  sensor      [20:18] %10lu  %10lu\r\n",
           (unsigned long)sensor, (unsigned long)p.f.sensor);
    printf("  long_addr   [28:21] %10lu  %10lu\r\n",
           (unsigned long)long_addr, (unsigned long)p.f.long_addr);
    printf("  short_addr  [30:29] %10lu  %10lu\r\n",
           (unsigned long)short_addr, (unsigned long)p.f.short_addr);
    printf("  addr_mode   [31]    %10lu  %10lu\r\n",
           (unsigned long)addr_mode, (unsigned long)p.f.addr_mode);

    int match = crc == p.f.crc && status == p.f.status &&
                payload == p.f.payload && battery == p.f.battery &&
                sensor == p.f.sensor && long_addr == p.f.long_addr &&
                short_addr == p.f.short_addr && addr_mode == p.f.addr_mode;
    printf("  both ways agree: %s\r\n", match ? "yes" : "NO — bug!");
  }
}
