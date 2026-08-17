# Nucleo-L476RG pinout diagrams

Morpho (CN7/CN10) + Arduino (CN5/CN6/CN8/CN9) connector diagrams per peripheral, from
<https://stm32python.gitlab.io/fr/docs/Stm32duino/projets/nucleo_l476rg_pins>
(downloaded 2026-08-17, derived from STM32duino `PeripheralPins.c`).

Verified against DS10198 (alternate-function tables) and UM1724 (board wiring):
button/LED, UART, I2C, SPI, PWM, and ADC mappings all match.

Notes:
- Tables show STM32duino's default pin picks — a subset; most signals have more AF
  options (e.g. PC8 carries TIM3_CH3 as well as the labeled TIM8_CH3).
- PA2/PA3 (USART2) are wired to the ST-LINK VCP, not the Arduino D0/D1 headers.
