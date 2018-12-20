# IBM-IR-PS2-stm32
An STM32F103-based receiver for the IBM SK-8807 wireless keyboard to PS/2

These infrared keyboards are sometimes available cheaply through surplus outlets--and aren't too bad
for rubber-dome models.  Using an inexpensive STM32F103-based "Blue Pill" or "Maple Mini" and an IR receiver
module (I used a Vishay TSOP4838 38KHz model) and a PS/2 keyboard cable, the IBM keyboard turns in a pretty
decent performance.  The batteries seem to last a very long time (still on my first set).

The output of the IR sensor is connected to USART3 RX and a debug output is available on USART1, which can also
be used for programming, depending on your setup (I used the STLINK V2 JTAG programmer, but every STM32F103 is
shipped with code for serial programming as well.   Supply for the MCU board comes from the PS/2 cable.

I used the Maple Mini boards (I have a small pile of them) and chose the PS/2 interface pins as PB6 and PB7 for 
clock and data, respectively--you can change the GPIO selections in the "gpiodef.h" file--just be sure to select
a pair of 5V tolerant pins on the same GPIO bus.  

I used the libopencm3 (available at http://libopencm3.org) hardware library, though it should be fairly straight-
forward to use any other library set (e.g. HAL, SPL, CubeMX, etc.).

Things I haven't done:  
  * Implemented the "nipple mouse".  Not something that I'm sure I'd use.
  * Implemented the "numeric keyboard simulation" key feature that maps a section of keys as a numeric keypad.
    Again, this is something that I have no use for, although financial types might like the feature.
    
I've tested this on several PCs running MSDOS, Windows XP and 7, as well as Ubuntu Linux.
