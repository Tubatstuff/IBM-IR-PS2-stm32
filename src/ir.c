//  Miscellaneous Utility Subroutines.
//  ----------------------------------


#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include "debug.h"
#include "gpiodef.h"
#include "globals.h"
#include "ir.h"

//*     SetupIRSensor - Set up UART3 for the IR sensor.
//      -----------------------------------------------
//
//      Basically, 1200 N81

void SetupIRSensor( void)
{

  IrRxBufferIn = 0;
  IrRxBufferOut = 0;		// make sure buffer is empty

  rcc_periph_clock_enable(RCC_USART3);
  nvic_enable_irq( NVIC_USART3_IRQ);
  usart_set_baudrate(USART3, 1200);	// keyboard is 1200 bps, N81
  usart_set_databits(USART3, 8);
  usart_set_stopbits(USART3, USART_STOPBITS_1);
  usart_set_mode(USART3, USART_MODE_RX);
  usart_set_parity(USART3, USART_PARITY_NONE);
  usart_set_flow_control(USART3, USART_FLOWCONTROL_NONE);
  USART_CR1(USART3) |= USART_CR1_RXNEIE;	// enable receive interrupt
  usart_enable( USART3);
} // SetupIRSensor

//*	USART3 (IR Sensor) Receive ISR
//	------------------------------
//
//	Just adds a character to the buffer, if possible.
//

void usart3_isr(void)
{

  if ( ((USART_CR1(USART3) & USART_CR1_RXNEIE) != 0) &&
       ((USART_SR(USART3) & USART_SR_RXNE) != 0)) 
  {

    int rxNext;

    IrRxBuffer[ IrRxBufferIn] = usart_recv( USART3);
    rxNext = IrRxBufferIn+1;
    if ( rxNext >= IR_RX_BUFFER_SIZE)
      rxNext = 0;			// wrap around
    if ( rxNext != IrRxBufferOut)
    {
      IrRxBufferIn = rxNext;	// stuff the new byte
    } 
  } // if we have a character.


} // usart3_isr

//  Sys_tick_handler - called every millisecond.
//  --------------------------------------------
//
//	also toggle the LED once per second.
//


void sys_tick_handler( void)
{
  TickCount++;

  if ( !(TickCount & 0x3ff))
  {  // Every 1024 milliseconds, blink LED
    gpio_toggle(LED_GPIO, LED_BIT); // LED on/off 
  }
} // sys_tick_handler

//*	SetupSysTick - Configure System Timer.
//	---------------------------------------
//
//	We interrupt every 1 msec.
//
  
void SetupSysTick( void)
{

// 72MHz / 8 => 9,000,000 counts per second 

  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

//  9000000/9000 = 1000 overflows per second - every 1ms one interrupt 

  systick_set_reload(8999);  //  SysTick interrupt every N clock pulses 
  systick_interrupt_enable();
  TickCount = 0;	     // clear tick counter
  systick_counter_enable();
} // SetupSysTick

