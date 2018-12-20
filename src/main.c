#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/usart.h>

#define MAIN

#include "gpiodef.h"
#include "globals.h"
#include "ps2.h"

#include "debug.h"

#include "ir.h"

//  Here's the lookup table for mapping IR keys to PS/2 keys.

#include "keymap.h"

static uint16_t GetIRByte( void);
static void ProcessKeys( void);
static void ProcessHostData( void);


//*  IBM IR keyboard to PS/2 Converter.
//   ----------------------------------
//
//    IBM wireless IR keyboards (SK-8807) can be had for cheap through
//    surplus outlets today.
//
//    This project uses a STM32F103 "Maple Mini" board.  A 38KHz IR receiver
//    is hooked to PA11 (MM pin 20).  The PS/2 keyboard is hooked to
//    PB6 (MM pin 16) for data and PB7 (MM pin 17) for clock.
//
//    If USART debug output is desired, it can be hooked to pins PA9 (MM 6)
//    for Tx and PA10 (MM 7) for Rx.
//
//    Status LEDs are on PA13-15 (MM pins 10-12).
//
//    No other connections are necessary, other than the connection from the 
//    PS/2  host for +5V and ground. .
//
//    We also provide status (scroll lock, num lock, caps lock) LEDs on
//    GPIOA13-15.
//
//    This project uses the libopencm3 library available at:
//
//	http://libopencm3.org/
//
//    Written by Chuck Guzis (chuck@sydex.com) in November, 2018.
// 
//    Things left undone:
//
//    Support of NUM key for numeric keypad functions (I never use it).
//    Support of the keyboard "mouse".  Maybe one of these days...
//
//    The PS/2 interface code here in (ps2.c) was based on Sebastian
//    Wicki's (gandro) stm32-ps2 code on github, which the author
//    gratefully acknowledges.
//

int main(void)
{

  rcc_clock_setup_in_hse_8mhz_out_72mhz();

// Enable GPIOC clock. 

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

// Set LED GPIO up.

  gpio_set_mode(LED_GPIO, GPIO_MODE_OUTPUT_2_MHZ,
      GPIO_CNF_OUTPUT_PUSHPULL, LED_BIT);

  gpio_clear(LED_GPIO, LED_BIT);	// LED on - blinks once per second

// Set status LEDs up.

  gpio_set_mode( STATUS_GPIO, GPIO_MODE_OUTPUT_2_MHZ,
      GPIO_CNF_OUTPUT_PUSHPULL, 
      STATUS_BIT_NUM | STATUS_BIT_SCROLL | STATUS_BIT_CAPS);
  gpio_clear( STATUS_GPIO, 
      STATUS_BIT_NUM | STATUS_BIT_SCROLL |  STATUS_BIT_CAPS);
  SetupSysTick();

//   The following is executed only if USART 1 debug output is desired.

  InitUART( 115200);
  Uprintf( "\nReady...\n");
  LastKey = 0;
  SetupIRSensor();
  PS2Init();			// start up the PS2 interface

//  ProcessKeys should never exit.

  ProcessKeys();

  return 0;
} // main


//*     GetIRByte - Get a byte from the IR receiver.
//      --------------------------------------------
//
//      Waits until a byte is ready.  If timeout, returns 0xffff;
//      otherwise returns the received byte.
//
//	The idea here is that the second byte in a 2-byte IR sequence
//	always follows immedately after the first, so if there's a 50
//	msec. or longer pause between characters, this can't be the first
//	byte of a 2-byte sequence.

#define KEY_TIMEOUT 50               // idle time in milliseconds

static uint16_t GetIRByte( void)
{
  
  uint32_t
    startTime;

  uint8_t
    cData;
    
  startTime = TickCount;        // start the timer
  while( (TickCount - startTime) < KEY_TIMEOUT)
  { // keep waiting for data
    if ( IrRxBufferIn != IrRxBufferOut)	// if there's data
    {
      cData = IrRxBuffer[ IrRxBufferOut++];	// get a byte from buffer
      if ( IrRxBufferOut >= IR_RX_BUFFER_SIZE)
        IrRxBufferOut = 0;	// wrap arond
      return cData;
    }	// if there's data in the buffer
  } // while we have time
  return 0xffff;                // timeout
} // GetIRByte

//*     ProcessKeys - Process IR keystrokes.
//      ------------------------------------
//
//      What all of this is about.  Basically, works like this:
//
//      1. Wait and read a byte from the IR sensor
//      2. If it's the mouse bye (0x3f), read two more, discard and go to 1.
//      3. Otherwise, read another byte and check validity against the first
//         byte.
//      4. Isolate the low-order 7 bits of the byte and look it up in the
//         keymap table.
//      5. If the keymap lookup returns a zero word, discard and go to 1
//      6. Otherwise, if the first byte received has the high order bit set
//         send a "key down" code.  If not, send a "key up" code. Go to 1.
//      
//      Note that on the PS/2, a "key up" prefixes an 0xf0 before the
//      last byte of a sequence.   For example, if the key down is E0 23,
//      the "key up" will bae E0 F0 23.
//

static void ProcessKeys( void)
{

  uint16_t
    rkey,
    b1,
    b2;
    
  const uint8_t
    pauseSeq[] = 
      {0xe1, 0x14, 0x77, 0xe1, 0xf0, 0x14, 0xf0, 0x77, 0 },
    pscrnMakeSeq[] = 
      {	0xe0, 0x12, 0xe0, 0x7c, 0 },
    pscrnBreakSeq[] =
      {	0xe0, 0xf0, 0x7c, 0xe0, 0xf0, 0x12, 0};
          

  while (1)
  { // servicing loop
  
//  See if there's data from the host.

    ProcessHostData();  
  
//  Okay, now look at the keyboard buffer.  

    b1 = GetIRByte();   // get first byte
    if (b1 == 0xffff)
      continue;                         // timeout
    b2 = GetIRByte();   // get second byte
    if ( b2 == 0xffff)
      continue;                         // timeout
    if ( b1 == IR_KEY_MOUSE)
    {  // someone touched the mouse
      GetIRByte();
      continue;                         // discard 2 bytes and loop
    } // if a mouse lead-in

//      Check to see that the two bytes agree.  If not, ignore.

    if ( b2 != ((~b1 & 0xf8) | (b1 & 0x7)) )
    { // if there
      continue;				// not valid--discard
    }   // if no match

// 	If this is a "make" code, save it for repeat.

    if ( b1 & 128)
      LastKey = b1;			// save it
    else
    { // check for repeat       
      if ( (b1 == IR_KEY_REPEAT) && LastKey)
        b1 = LastKey;			// save last key
      else
        LastKey = 0;			// any other release clears
    } // if not a make	

//	Check for Pause/Break and Print Screen.
        
    if ( b1 == (IR_KEY_PAUSE | 128))
      PS2PutStr( (uint8_t *) pauseSeq);		// Pause has no break
    else if ( b1 == (IR_KEY_PRTSCRN | 128))
      PS2PutStr( (uint8_t *) pscrnMakeSeq);
    else if ( b1 == IR_KEY_PRTSCRN) 
      PS2PutStr( (uint8_t *) pscrnBreakSeq); 

    rkey = KeyMap[ b1 & 127];         // get the result key
    if (!rkey)
      continue;                         // if a null key mapping

    if ( rkey & 0xff00)
    {   // first part of 2-byte code
      PS2Put( rkey >> 8);
    }
    if ( !(b1 & 128))
    {   // key up
      PS2Put( 0xf0);
      LastKey = 0;
    }
    PS2Put( rkey & 0xff);
  } // while
  return;
} // ProcessKeys

// 	ProcessHostData - Check for messages coming from the host.
//      ----------------------------------------------------------
//
//	Usually, the response to these messages is an ACK, but there
//	some exceptions.  In all cases, since we don't have LEDs, nor
//	an adjustable typematic rate, we don't really do anything other
//	than to say "I got it".
//

static void ProcessHostData( void)
{

  int 
    ps2val,
    param;
  uint32_t
    startTime;

#define HOST_TIMEOUT 5		// 5 msec should be more than enough
    
  if ( (ps2val = PS2Get()) == -1)
    return;		// nothing to do

Uprintf( "Host sends %02x ", ps2val);
    
  switch( ps2val)
  {
   
    case HOST_RESET:
      PS2Put( KEY_ACK);
      PS2Put( KEY_BAT);		// say reset's done
      UpdateStatusLEDs( 0);	// turn the LEDs off
      break;
        
    case HOST_DEFAULT:
    case HOST_DISABLE:
    case HOST_ENABLE:
      PS2Put( KEY_ACK);		// just acknowledge
      break;
    
    case HOST_ECHO:
      PS2Put( KEY_ECHO);	// respond with echo
      break;

    case HOST_ID:		// get keyboard ID
      PS2Put( KEY_ACK);
      PS2Put( 0x83);		// default 101-key
      PS2Put( 0xab);
      break; 
      
    case HOST_TYPEMATIC:	// we need another byte
    case HOST_SET_SCAN:
    case HOST_SET_LED:
      startTime = TickCount;        // start the timer
      do
      {
        if ( (TickCount - startTime) > HOST_TIMEOUT)
          break;		// we must have missed it        
      } while ( (param = PS2Get()) == -1);
      
      if ( ps2val == HOST_SET_LED)
        UpdateStatusLEDs( (uint8_t) param);// update the LEDs      
      PS2Put( KEY_ACK);		// just acknowledge it
      break;      

    default:
      Uprintf( "Unknown code %02x\n", ps2val);
      PS2Put( KEY_RESEND);	// don't know what it is
      break;
  
  } // get request type
  Uprintf( "\n"); 
  return;
} //  ProcessHostData
