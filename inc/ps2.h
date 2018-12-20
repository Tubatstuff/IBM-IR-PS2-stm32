#ifndef __PS2_H__
#define __PS2_H__

// Codes sent by host to keyboard.

#define HOST_RESET 0xff			// reset
#define HOST_RESEND 0xfe		// resend last code
#define HOST_DEFAULT 0xf6		// set defaults
#define HOST_DISABLE 0xf5		// stop keyboard
#define HOST_ENABLE 0xf4		// start keyboard
#define HOST_ID 0xf2			// read keyboard ID
#define HOST_ECHO 0xee			// diagnostic echo

// The following are 2-byte codes

#define HOST_TYPEMATIC 0xf3		// Set typematic rate/delay
#define HOST_SET_LED 0xed		// set/reset LED
#define HOST_SET_SCAN 0xf0		// set scan set

// Codes sent by keyboard to host in response to above.

#define KEY_RESEND 0xfe			// resend previous code
#define KEY_ACK 0xfa			// Acknowledge
#define KEY_OVERRUN 0x00		// signal buffer full
#define KEY_FAILURE 0xfd		// diagnostic failure
#define KEY_BAT 0xaa			// BAT completed
#define KEY_ECHO 0xee			// response to echo

//	Prototypes.

void UpdateStatusLEDs( uint8_t What);
void PS2Init( void);
int PS2Ready( void);
void PS2Put( uint8_t What);
void PS2PutStr( uint8_t *What);
int PS2Get( void);

#endif
