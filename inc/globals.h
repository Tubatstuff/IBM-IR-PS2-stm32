#ifndef _globals_defined
#define _globals_defined
#include <stdint.h>

#ifdef MAIN
#define _scope 
#else
#define _scope extern
#endif

#define TRUE 1
#define FALSE 0

_scope uint16_t
  LastKey;		// Last key pressed

_scope volatile uint32_t 
  TickCount;            // incremented by systick - 1.0  ms.

_scope uint8_t 
  LastMake;		// last "make"

#define IR_RX_BUFFER_SIZE 64	// how many bytes in the IR receive buffer

_scope volatile int
  IrRxBufferIn,
  IrRxBufferOut;
  
_scope uint8_t
  IrRxBuffer[ IR_RX_BUFFER_SIZE];

#define PS2_RX_BUFFER_SIZE 64	// how many bytes in the ps2 receive buffer

_scope volatile int
  PS2RxBufferIn,
  PS2RxBufferOut;
  
_scope uint8_t
  PS2RxBuffer[ PS2_RX_BUFFER_SIZE];	// the buffer

#endif

