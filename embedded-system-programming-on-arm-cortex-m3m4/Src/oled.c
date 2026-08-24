/* Side demo: a 128x32 OLED module (usually sold as an "LCD", actually an
 * OLED with an SSD1306 controller chip) — the counterpart to the 7-segment
 * demo.
 *
 * On the 7-segment display WE were the display driver: every LED hung
 * directly off a GPIO bit. This module is the other way of building
 * hardware: the 4096 pixels belong to the SSD1306 chip ON the module,
 * which has its own RAM (one bit per pixel) and its own command set.
 * None of that appears in our 4 GB memory map — the SSD1306 is outside
 * the microcontroller, so no pointer can reach it. The only thing in OUR
 * map is I2C3 (0x4000_5C00, APB1), and we reach the display's registers
 * by sending bytes THROUGH it. Two levels of "register" at once:
 *
 *   our map:  I2C3 TXDR etc.  — written with a pointer, rides APB1
 *   its chip: commands + RAM  — written with a protocol, rides 2 wires
 *
 * Wiring (4 pins — the whole point of a controller chip):
 *   module VCC -> 3V3, GND -> GND, SCL -> A5 (PC0), SDA -> A4 (PC1)
 * Coexists with the 7-segment wiring; nothing overlaps.
 *
 * Every I2C write starts with a control byte: 0x00 = "command follows",
 * 0x40 = "pixel data follows". We keep a 512-byte framebuffer in OUR SRAM
 * (128 x 32 pixels / 8 per byte), draw text into it, then ship it over.
 * The demo probes address 0x3C/0x3D until the module answers, so you can
 * wire it up with the firmware already running. Shows a title, our I2C3
 * peripheral's address, an uptime counter, and echoes what you type in
 * the serial terminal.
 *
 * When you'll use this: most of real embedded work is exactly this — a
 * smarter chip on the far side of a bus (sensors, EEPROMs, radios,
 * displays), driven by the same probe / init-sequence / transfer routine.
 * Learn the pattern once here and every I2C datasheet's "write this
 * register, then that data" section reads as familiar. */

#include "oled.h"

#include "i2c3.h"
#include "uart2.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define OLED_PAGES 4U /* 32 rows = 4 stacked 8-pixel "pages" */
#define OLED_COLS 128U

static uint8_t framebuffer[OLED_PAGES * OLED_COLS];
static uint8_t oled_addr; /* 0x3C or 0x3D, whichever answers */

/* 5x7 font, ASCII 32..90 (uppercase only; lowercase gets folded).
 * Each glyph is 5 column bytes, bit 0 = top pixel. */
static const uint8_t FONT[59][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

static int oled_cmd(uint8_t cmd) {
    uint8_t packet[2] = {0x00, cmd}; /* 0x00 = "next byte is a command" */
    return i2c3_write(oled_addr, packet, 2);
}

/* the SSD1306 power-up recipe for the 128x32 panel, from its datasheet */
static const uint8_t INIT_CMDS[] = {
    0xAE, /* display off while we configure          */
    0xD5,
    0x80, /* internal clock, default divide          */
    0xA8,
    0x1F, /* multiplex 31 -> 32 rows                 */
    0xD3,
    0x00, /* no vertical offset                      */
    0x40, /* start scanning at RAM line 0            */
    0x8D,
    0x14, /* charge pump ON (module has no Vcc rail) */
    0x20,
    0x00, /* horizontal addressing: RAM pointer walks
             left->right then next page — lets us ship
             the whole framebuffer in a burst         */
    0xA1, /* flip left-right  (module sits pins-up)  */
    0xC8, /* flip top-bottom                         */
    0xDA,
    0x02, /* COM wiring for the 128x32 panel         */
    0x81,
    0x8F, /* contrast                                */
    0xD9,
    0xF1, /* pixel pre-charge timing                 */
    0xDB,
    0x40, /* pixel voltage level                     */
    0xA4, /* show RAM contents (not all-on test)     */
    0xA6, /* normal polarity (1 = lit)               */
    0xAF, /* display on                              */
};

static int oled_init_display(void) {
    for (uint32_t i = 0; i < sizeof(INIT_CMDS); ++i) {
        if (oled_cmd(INIT_CMDS[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

/* ship the framebuffer: aim the chip's RAM pointer at the top-left, then
 * 4 bursts of 128 pixel-bytes, each prefixed with the 0x40 control byte */
static void oled_flush(void) {
    static uint8_t chunk[1 + OLED_COLS];

    oled_cmd(0x21);
    oled_cmd(0x00);
    oled_cmd(0x7F); /* columns 0..127 */
    oled_cmd(0x22);
    oled_cmd(0x00);
    oled_cmd(0x03); /* pages 0..3     */

    for (uint32_t page = 0; page < OLED_PAGES; ++page) {
        chunk[0] = 0x40; /* "pixel data follows" */
        memcpy(&chunk[1], &framebuffer[page * OLED_COLS], OLED_COLS);
        i2c3_write(oled_addr, chunk, sizeof(chunk));
    }
}

/* draw text into the framebuffer at a page (0..3) and pixel column */
static void oled_text(uint32_t page, uint32_t col, const char* text) {
    for (; *text != '\0'; ++text) {
        char letter = *text;
        if (letter >= 'a' && letter <= 'z') {
            letter = (char)(letter - 'a' + 'A'); /* uppercase-only font */
        }
        if (letter < ' ' || letter > 'Z' || col + 5U > OLED_COLS) {
            continue;
        }
        memcpy(&framebuffer[(page * OLED_COLS) + col], FONT[letter - ' '], 5);
        col += 6U; /* 5 columns of glyph + 1 blank */
    }
}

static void hold_a_moment(void) {
    for (volatile uint32_t i = 0; i < 700U; ++i) {} /* ~2 ms at 4 MHz */
}

void playing_with_the_oled(void) {
    char line[22];
    uint32_t seconds = 0;
    uint32_t ticks = 0;
    uint32_t echo_col = 12; /* after the "> " prompt */

    uart2_init();
    i2c3_init();

    printf("\r\nSide demo: 128x32 OLED — pixels that live OUTSIDE the memory map\r\n");
    printf("our end: I2C3 @0x40005C00 (APB1), pins PC0/A5=SCL PC1/A4=SDA\r\n");

    /* keep knocking until the module answers — wire it up any time */
    for (;;) {
        if (i2c3_write(0x3C, NULL, 0) == 0) {
            oled_addr = 0x3C;
        } else if (i2c3_write(0x3D, NULL, 0) == 0) {
            oled_addr = 0x3D;
        }
        if (oled_addr != 0 && oled_init_display() == 0) {
            break;
        }
        oled_addr = 0;
        printf("no ACK at 0x3C/0x3D — check VCC=3V3 GND SCL=A5 SDA=A4, retrying\r\n");
        for (uint32_t i = 0; i < 1000U; ++i) {
            hold_a_moment(); /* ~2 s between knocks */
        }
    }
    printf("SSD1306 answered at 0x%02X — its RAM is ours now\r\n", oled_addr);

    memset(framebuffer, 0, sizeof(framebuffer));
    oled_text(0, 0, "SSD1306 128X32 OLED");
    oled_text(1, 0, "VIA I2C3 @40005C00");
    oled_text(3, 0, "> ");
    oled_flush();

    for (;;) {
        int key = uart2_poll();
        if (key >= ' ' && key < 0x7F) {
            if (echo_col + 6U > OLED_COLS) { /* line full: start it over */
                memset(&framebuffer[(3U * OLED_COLS) + 12U], 0, OLED_COLS - 12U);
                echo_col = 12;
            }
            char typed[2] = {(char)key, '\0'};
            oled_text(3, echo_col, typed);
            echo_col += 6U;
            printf("'%c' -> APB1 -> I2C3 -> SDA/SCL -> SSD1306 RAM\r\n", key);
            oled_flush();
        }

        if (++ticks >= 500U) { /* ~1 s of 2 ms holds */
            ticks = 0;
            ++seconds;
            (void)snprintf(line, sizeof(line), "UP %lu S", (unsigned long)seconds);
            memset(&framebuffer[2U * OLED_COLS], 0, OLED_COLS);
            oled_text(2, 0, line);
            oled_flush();
        }
        hold_a_moment();
    }
}
