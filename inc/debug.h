// Debug definitions.
// ------------------

#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED 1

//	If you want debug messaging enabled via USART1,
//	uncomment the following line.

// #define USE_USART_DEBUG 1

#ifdef USE_USART_DEBUG
#include "uart.h"
#else
#define Uprintf(fmt...)		// so nothing prints
#define InitUART( bitrate)	// no-op the initialize
#endif
#endif // DEBUG_INCLUDED