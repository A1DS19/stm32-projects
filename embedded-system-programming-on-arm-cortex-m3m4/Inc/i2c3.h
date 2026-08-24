#ifndef I2C3_H
#define I2C3_H

#include <stdint.h>

/* Register-level I2C3 master on PC0 (SCL, Arduino A5) / PC1 (SDA, A4),
 * ~100 kHz. Write-only — enough to drive an SSD1306 display. */
void i2c3_init(void);

/* Write len bytes to a 7-bit address (len 0 probes for an ACK).
 * 0 = acknowledged, -1 = NACK, -2 = bus timeout. */
int i2c3_write(uint8_t addr7, const uint8_t* data, uint32_t len);

#endif
