#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "globals.h"
#include "gpiodef.h"
#include "debug.h"
#include "ps2.h"

//*	PS2 Key-Host communication.
//	---------------------------
//
//	Most of this code is driven by a 12MHz counter in up-down counting
//	mode, with compare interrupts at different points in the counting
//	sequence.   Overall, a PS/2 clock train is generated at a frequency
//	of about 11KHz.   The compare points generte interrupts so that
//	data pulses may be either output or sampled at appropriate places
//	within the 11KHz pulse train.
//
//	Data received from the host is placed in a 64-byte buffer.  Since
//	IR keyboard data is also buffered, keyboard-to-host communication
//	is not buffered.
//
//	Be careful where you put debug output calls--there's a good chance
//	that you could mess up the timing, so be careful.  Debug output
//	is done through a buffered interrupt-driven routine, so you should
//	be fine if you keep your debug messages terse.
//

//  Sates for receive and transmit.

typedef enum 
{ 
  IDLE, 
  SEND, 
  REQUEST, 
  RECEIVE 
} PS2_STATE;

// Data transfer states.

typedef enum 
{ 
  START, 
  DATA, 
  PARITY, 
  STOP, 
  ACK,
  UNACK, 
  FINISHED 
} PS2_TRANSFER_STATE;

static volatile PS2_STATE 
  PS2State;

static volatile PS2_TRANSFER_STATE 
  PS2TransferState;

static int 
  PS2Prescaler,			// TIM2 prescaler
  PS2Period,			// TIM2 period
  PS2SendRequest;

static  uint8_t 
  PS2OutputData,
  PS2OutputBitPos,
  PS2InputData,
  PS2InputBitPos,
  PS2Parity;

//  Local prototypes.

static PS2_TRANSFER_STATE SendDataIRQHandler(void);
static PS2_TRANSFER_STATE ReceiveDataIRQHandler(void);
static void DataIRQHandler(void);
static void CheckReceiveRequest(void);
static void CheckSendRequest(void);
static void SendClear(void);
static void ReceiveClear(void);
static void ClockIRQHandler(void);

//*	UpdateStatusLEDs - Update Status LEDs.
//	--------------------------------------
//
//	On entry, the LED status: xxxxxCNS
//

void UpdateStatusLEDs( uint8_t What)
{

  if ( What & 1)
    gpio_set(STATUS_GPIO, STATUS_BIT_SCROLL);
  else
    gpio_clear(STATUS_GPIO, STATUS_BIT_SCROLL);
  
  if ( What & 2)
    gpio_set(STATUS_GPIO, STATUS_BIT_NUM);
  else
    gpio_clear(STATUS_GPIO, STATUS_BIT_NUM);
  
  if ( What & 4)
    gpio_set(STATUS_GPIO, STATUS_BIT_CAPS);
  else
    gpio_clear(STATUS_GPIO, STATUS_BIT_CAPS);
  
  return;
} //  UpdateStatusLEDs

//*	PS2Ready - Return interface state.
//	----------------------------------
//
//	Returns 1 if interface is idle. 0 otherwise.
//

int PS2Ready( void)
{

  return ( PS2State == IDLE) ? 1 : 0;
} // PS2Ready


//*	PS2Put - Put a character to interface.
//	--------------------------------------
//
//	If interface is busy, this will stall until ready.
//

void PS2Put( uint8_t What)
{
  while( !PS2Ready()) {};	// stall until ready
  PS2OutputData = What;		// what we're sending
  PS2SendRequest = 1;		// say we need to send something
  while( PS2Ready()) {};	// wait for the timer to pick it up
  return;
} // PS2Put

//	PS2PutStr - Put a null-terminated string to interface.
//	------------------------------------------------------
//
//	Just calls PS2Put().
//

void PS2PutStr( uint8_t *What)
{
  
  do
  {
    PS2Put( *What++);
  } while ( *What);
  return;
} // PS2PutStr

//*	PS2Get - Get a character from interface.
//	----------------------------------------
//
//	Returns -1 if no data available; otherwise
//	returns data last read.
//

int PS2Get( void)
{
  
  int 
   idata;
   
  if ( !PS2Ready()) 
    return -1;			// if busy
  if (PS2RxBufferIn == PS2RxBufferOut)
    return -1;			// empty
  idata = PS2RxBuffer[ PS2RxBufferOut++];	// get data
  if ( PS2RxBufferOut >= PS2_RX_BUFFER_SIZE)
    PS2RxBufferOut = 0;        // wrap arond
  return idata;
} // PS2Get


//* 	PS2Init - Initialize PS2 GPIO and Timer
//  	---------------------------------------
//
//  	We use PB0 and PB1 for clock and data and TIM2 for timer.
//  	This whole affair is driven by TIM2 running at about 12KHz.
//  	We use the up+down so that we can sample in the middle of a
//  	bit cell.
//

void PS2Init( void)
{

//  Setup the clock and data GPIO pins.  GPIO open-drain and high.

  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_set_mode( PS2_GPIO, GPIO_MODE_OUTPUT_50_MHZ, 
    GPIO_CNF_OUTPUT_OPENDRAIN, PS2_BIT_CLK | PS2_BIT_DATA);

  gpio_set( PS2_GPIO, PS2_BIT_CLK | PS2_BIT_DATA);
    
//  Handle the setup for TIM2.

  PS2Prescaler = 3;
  PS2Period = 2000;		// should be 12KHz

  nvic_enable_irq(NVIC_TIM2_IRQ);	// enable interrupt
  rcc_periph_clock_enable(RCC_TIM2);
  rcc_periph_reset_pulse(RST_TIM2);
  timer_set_prescaler( TIM2,  PS2Prescaler - 1);
  timer_set_period( TIM2, (PS2Period / 2) - 1);
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_CENTER_3, TIM_CR1_DIR_UP);
  timer_disable_preload(TIM2);
  timer_continuous_mode( TIM2);
  
//  Now for TIM2 compare 1 mode.

  timer_set_oc_mode( TIM2, TIM_OC1, TIM_OCM_FROZEN);
  timer_enable_oc_output( TIM2, TIM_OC1);
  timer_set_oc_polarity_high( TIM2, TIM_OC1);
  timer_set_oc_value( TIM2, TIM_OC1, (PS2Period / 4) - 1); 
  timer_disable_oc_preload( TIM2, TIM_OC1);
    
//  And then the TIM2 compare 2 mode.

  timer_set_oc_mode( TIM2, TIM_OC2, TIM_OCM_FROZEN);
  timer_enable_oc_output( TIM2, TIM_OC2);
  timer_set_oc_polarity_high( TIM2, TIM_OC2);
  timer_set_oc_value( TIM2, TIM_OC2, (PS2Period / 2) - 1); 
  timer_disable_oc_preload( TIM2, TIM_OC2);

//  enable interrupts for OC1 and OC2.

  timer_enable_irq( TIM2, TIM_DIER_CC1IE | TIM_DIER_CC2IE);
  
//  Set up the various state variables.

  PS2State = IDLE;
  PS2TransferState = START;
  PS2SendRequest = 0;
  PS2OutputData = 0x00,
  PS2OutputBitPos = 0,
  PS2InputData = 0,
  PS2InputBitPos = 0,
  PS2Parity = 0;

//  The next two are debug

  PS2RxBufferIn = 0;
  PS2RxBufferOut = 0;		// clear the buffer

//  Delay 300 msec, then send the BAT code.

  TickCount = 0;
  while (TickCount < 300) {}
  
// Enable the timer and send the BAT complete code.  

  timer_enable_counter( TIM2);
  PS2Put( 0xaa);
  return;
} // PS2Init


//  SendDataIRQHandler - Handler for sending data.
//  ----------------------------------------------
//
//

static PS2_TRANSFER_STATE SendDataIRQHandler(void)
{
  int 
    PS2DataBit = 0;
  PS2_TRANSFER_STATE 
    PS2NextState = PS2TransferState;

  switch(PS2TransferState) 
  {
    case START:				// initialize
      PS2OutputBitPos = 0;
      PS2Parity = 0;

//	Send start bit

      PS2DataBit = 0;
      PS2NextState = DATA;
      break;

    case DATA:				// next bit 
      PS2DataBit = (PS2OutputData >> PS2OutputBitPos) & 1;
      PS2OutputBitPos++;

      PS2Parity ^= PS2DataBit;	// compute parity
      if(PS2OutputBitPos > 7) 
      {  // finalize parity bit
        PS2Parity ^= 1;		// toggle it
        PS2NextState = PARITY;
      }
      break;

    case PARITY:			// send parity bit
      PS2DataBit = PS2Parity;
      PS2NextState = STOP;
      break;
        
    case STOP:				// send stop bit
      PS2DataBit = 1;
      PS2NextState = FINISHED;
      break;
        
    case ACK:
    case UNACK:
    case FINISHED:			// impossible state
      break;
  } // PS2NextState

//    Write output bit 

  if ( PS2DataBit)
    gpio_set( PS2_GPIO, PS2_BIT_DATA);
  else
    gpio_clear( PS2_GPIO, PS2_BIT_DATA);
  return PS2NextState;
} //   PS2 Send IRQ Data Handler


//	PS/2 Data receive ISR
//	---------------------
//
//

static PS2_TRANSFER_STATE ReceiveDataIRQHandler(void)
{

  PS2_TRANSFER_STATE 
    PS2NextState = PS2TransferState;

  int 
    ps2DataBit,
    rxNext;

//	Get bit from port.
  
  ps2DataBit = gpio_get( PS2_GPIO, PS2_BIT_DATA) ? 1 : 0;
  switch( PS2TransferState) 
  {
    case START:		// Got the first bit
      PS2InputBitPos = 0;
      PS2Parity = 0;
      if(ps2DataBit != 0) 
          break;	// this shouldn't happen, so ignore the bit

      PS2NextState = DATA;
      PS2InputData = 0;	// clear the input accumulator
      break;

    case DATA:		// got data bits
      PS2InputData |= (ps2DataBit << PS2InputBitPos);
      PS2InputBitPos++;
      PS2Parity ^= ps2DataBit;
      if (PS2InputBitPos > 7) 
      { // end of string, so flip parity
         PS2Parity ^= 1;
         PS2NextState = PARITY;
      }
      break;
        
    case PARITY:
      if( ps2DataBit != PS2Parity) 
      { // Parity error, here, just ignore for now
      }
      PS2NextState = STOP;
      break;
        
    case STOP:
      if (!ps2DataBit) 
      { // didn't get the stop bit--ignore
      }
      PS2NextState = ACK;
      break;
        
    case ACK:		// set an acknowledge out
      gpio_clear( PS2_GPIO, PS2_BIT_DATA);
      PS2NextState = UNACK;		// finish up
      break;
      
    case UNACK:
      gpio_set( PS2_GPIO, PS2_BIT_DATA);
      PS2RxBuffer[ PS2RxBufferIn] = PS2InputData;
      rxNext = PS2RxBufferIn+1;
      if ( rxNext >= PS2_RX_BUFFER_SIZE)
        rxNext = 0;                   // wrap around
      if ( rxNext != PS2RxBufferOut)
        PS2RxBufferIn = rxNext;    // stuff the new byte
      PS2NextState = FINISHED;
      break;

    case FINISHED:	// finally, release the ACK and stash buffer
      break;
  
  } // switch

  return PS2NextState;
} // ReceiveDataIRQHandler

//  DataIRQHandler - Advance State if possible.
//  -------------------------------------------
//  
//	Invoked from tim2_isr.
//

static void DataIRQHandler(void)
{

  if (PS2State == IDLE || PS2State == REQUEST) 
    return;		// nothing to do

// See if the communication was canceled 

  if ( !gpio_get(PS2_GPIO, PS2_BIT_CLK) ) 
  { // Release DATA Pin 
    gpio_set(PS2_GPIO, PS2_BIT_DATA);
    PS2State = IDLE;
    return;
  }

//  Dispatch to appropriate send/receive

  if (PS2State == SEND) 
    PS2TransferState = SendDataIRQHandler();
  else if (PS2State == RECEIVE) 
    PS2TransferState = ReceiveDataIRQHandler();
  return;
} // PS2_DataIrqHandler

//  CheckReceiveRequest - Begin receive state.
//  -------------------------------------------
//
//   Invoked from tim2_isr.
//

static void CheckReceiveRequest(void)
{

  if(PS2State == IDLE) 
  { // Idle, Clock should be set, otherwise we have a receive request 
    if( !gpio_get(PS2_GPIO, PS2_BIT_CLK ) )
      PS2State = REQUEST;
  } else if( PS2State == REQUEST) 
  { // Check if CLK is set again, then the transfer can start 
    if( gpio_get(PS2_GPIO, PS2_BIT_CLK) ) 
    {  // clock high?
      if ( !gpio_get( PS2_GPIO, PS2_BIT_DATA) )
      { // Data low
        PS2State = RECEIVE;
        PS2TransferState = START;
      } // if data high
      else
        PS2State = IDLE;	// clock and data both high, forget it
    }  // clock line high?
  } // see if transfer can start
  return;
} // CheckReceiveRequest


//  CheckSendRequest - Start a sending sequence.
//  --------------------------------------------
//
//   Invoked from tim2_isr.
//

static void CheckSendRequest(void)
{
  if(PS2State == IDLE && PS2SendRequest) 
  {  // has to be idle to start sending
    PS2State = SEND;
    PS2TransferState = START;
  }
  return;
} // CheckSendRequest


//  SendClear - Transition from SEND to IDLE state
//  ----------------------------------------------
//
//  Invoked from tim2_isr; transitions to IDLE state
//  if sending complete.
//

static void SendClear(void)
{

  if(PS2State == SEND && PS2TransferState == FINISHED) 
  {
    PS2State = IDLE;
    PS2SendRequest = 0;
  }
  return;
} // SendClear

//  ReceiveClear - Transition to IDLE state
//  ---------------------------------------
//
//	Invoked from tim2_isr.
//

static void ReceiveClear(void)
{

  if ( (PS2State == RECEIVE) && (PS2TransferState == FINISHED)) 
  {
    gpio_set( PS2_GPIO, PS2_BIT_CLK | PS2_BIT_DATA);	// release both lines
    PS2State = IDLE;
  }
  return;
} // ReceiveClear

//  ClockIRQHandler - Handle interrupt on clock transition.
//  -------------------------------------------------------
//
//  Called from tim2_isr
//

static void ClockIRQHandler(void)
{

  if ( !(TIM_CR1(TIM2) & TIM_CR1_DIR_DOWN))
  { // counter is counting up.
    CheckReceiveRequest();
    if(PS2State == SEND || PS2State == RECEIVE) 
      gpio_set(PS2_GPIO, PS2_BIT_CLK);	// positive CLK
    if (PS2State == SEND)
      SendClear();
    CheckSendRequest();
  } else 
  { // Counter Direction DOWN, CLK Falling Edge 
    if(PS2State == SEND || PS2State == RECEIVE) 
      gpio_clear(PS2_GPIO, PS2_BIT_CLK);  // neagive Clk
    ReceiveClear();
  } // if counting down
  return;
} // ClockIRQHandler

//	Interrupt handler.
//	------------------

void tim2_isr(void)
{
  if ( timer_get_flag(TIM2, TIM_SR_CC1IF)) 
  { // CC1 is the CLK Timer Channel 
   timer_clear_flag(TIM2, TIM_SR_CC1IF);
   ClockIRQHandler();
  } else if (timer_get_flag(TIM2, TIM_SR_CC2IF)) 
  { //  CC2 is the DATA Timer Channel 
    timer_clear_flag(TIM2, TIM_SR_CC2IF);
    DataIRQHandler();
  }
} // TIM2_IRQHandler

