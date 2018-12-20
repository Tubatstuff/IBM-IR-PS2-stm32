#ifndef __GPIODEFS_H__
#define __GPIODEFS_H__ 1

//   Usage of various GPIO pins.  Note that for the PS/2 clock and data,
//   you must select 5V tolerant GPIO pins.

#define PS2_GPIO        GPIOB
#define PS2_BIT_CLK     GPIO6           // GPIO bit 7
#define PS2_BIT_DATA    GPIO7           // GPIO bit 6

//  "Pulse" LED, blinks once per second.

#define LED_GPIO GPIOB          // GPIO for LED
#define LED_BIT  GPIO1          // Bit in GPIO for led

//  Status LEDs.

#define STATUS_GPIO GPIOA       // GPIO for status LEDs
#define STATUS_BIT_NUM GPIO13   // Numlock status
#define STATUS_BIT_SCROLL GPIO14 // Scroll Lock status
#define STATUS_BIT_CAPS GPIO15  // Caps lock status

//  Note that USART1 (for debug) and USART3 (IR input) have
//  their own pins defined in libopencm3, so they're not defined here.

#endif
