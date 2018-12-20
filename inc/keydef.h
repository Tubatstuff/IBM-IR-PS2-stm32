#ifndef _KEYDEFS_INCLUDED_
#define _KEYDEFS_INCLUDED_

//	Key definitions.   PS/2 Set 2 Scancodes.
//	This is used to define the correspondence between the IR keyboard
//	keys and the scan codes.   So, if you get a code from the IR keyboard
//	(strip high-order bit), you index into keymap and retrieve the PS/2
//	scan codes.  Unmapped keys are defined as KEY_NULL and generate no
//      scan codes (i.e., they're "dead" keys).
//
//	Note that special-casing in the code is done for PAUSE, PRINT SCREEN
//	and mouse codes.

#define  KEY_NULL 		0x0000		// null key--no output
#define  KEY_UP_ARROW		0xe075
#define  KEY_NUM		KEY_NULL
#define  KEY_END		0xe069
#define  KEY_HOME		0xe06c
#define  KEY_PAGE_DOWN		0xe07a
#define  KEY_PAGE_UP		0xe07d
#define  KEY_DOWN_ARROW		0xe072
#define  KEY_PRINT		0xe07c		// really e012e07c
#define  KEY_RIGHT_ARROW	0xe074
#define  KEY_INSERT		0xe070
#define  KEY_GREEN		KEY_NULL	// nothing yet
#define  KEY_BLACK		KEY_NULL	
#define  KEY_YELLOW		KEY_NULL
#define  KEY_LIGHT_BLUE		KEY_NULL
#define  KEY_LEFT_SCREEN	KEY_NULL
#define  KEY_DARK_BLUE		KEY_NULL
#define  KEY_WHITE		0xe05f		// Sleep function
#define  KEY_RIGHT_SCREEN	KEY_NULL
#define  KEY_LEFT_ARROW		0xe06b
#define  KEY_HOUSE		0xe05b		// use as left Window
#define  KEY_DELETE		0xe071
#define  KEY_F4			0x0c
#define  KEY_F3			0x04
#define  KEY_F2			0x06
#define  KEY_F1			0x05
#define  KEY_F8			0x0a
#define  KEY_F7			0x83
#define  KEY_F6			0x0b
#define  KEY_F5			0x03
#define  KEY_F12		0x07
#define  KEY_F11		0x78
#define  KEY_F10		0x09
#define  KEY_F9			0x01
#define  KEY_BREAK		0xe114		// more complicated
#define  KEY_SCROLL_LOCK	0x7e
#define  KEY_NUM_LOCK		0x77
#define  KEY_LARGE_LEFT_BUTTON  KEY_NULL
#define  KEY_SMALL_LEFT_BUTTON	KEY_NULL
#define  KEY_ESC		0x76
#define  KEY_E			0x24
#define  KEY_W			0x1d
#define  KEY_Q			0x15
#define  KEY_TAB		0x0d
#define  KEY_U			0x3c
#define  KEY_Y			0x35
#define  KEY_T			0x2c
#define  KEY_R			0x2d
#define  KEY_OPEN_BRACKET	0x54
#define  KEY_P			0x4d
#define  KEY_O			0x44
#define  KEY_I			0x43
#define  KEY_A			0x1c
#define  KEY_CAPS_LOCK		0x58
#define  KEY_BACKSLASH		0x5d
#define  KEY_CLOSE_BRACKET	0x5b
#define  KEY_2			0x1e
#define  KEY_1			0x16
#define  KEY_GRAVE		0x0e
#define  KEY_6			0x36
#define  KEY_5			0x2e
#define  KEY_4			0x25
#define  KEY_3			0x26
#define  KEY_0			0x45
#define  KEY_9			0x46
#define  KEY_8			0x3e
#define  KEY_7			0x3d
#define  KEY_BACKSPACE		0x66
#define  KEY_EQUALS		0x55
#define  KEY_HYPHEN		0x4e
#define  KEY_N			0x31
#define  KEY_B			0x32
#define  KEY_V			0x2a
#define  KEY_C			0x21
#define  KEY_FORWARD_SLASH	0x4a
#define  KEY_PERIOD		0x49
#define  KEY_COMMA		0x41
#define  KEY_M			0x3a
#define  KEY_LEFT_CTRL		0x14
#define  KEY_RIGHT_SHIFT	0x59
#define  KEY_RED		KEY_NULL
#define  KEY_VIOLET		KEY_NULL
#define  KEY_SPACE		0x29
#define  KEY_LEFT_ALT		0x11
#define  KEY_G			0x34
#define  KEY_F			0x2b
#define  KEY_D			0x23
#define  KEY_S			0x1b
#define  KEY_L			0x4b
#define  KEY_K			0x42
#define  KEY_J			0x3b
#define  KEY_H			0x33
#define  KEY_ENTER		0x5a
#define  KEY_APOSTROPHE		0x52
#define  KEY_SEMICOLON		0x4c
#define  KEY_X			0x22
#define  KEY_Z			0x1a
#define  KEY_LEFT_SHIFT		0x12

//  Special IR keys definitions.

#define IR_KEY_MOUSE    0x3f            // mouse lead in
#define IR_KEY_REPEAT   0x53            // typematic repeat
#define IR_KEY_CLEAR    0x5d            // all keys up
#define IR_KEY_PRTSCRN  0x09		// print screen
#define IR_KEY_PAUSE	0x2d		// break/pause

#endif		// _KEYDEFS_INCLUDED_
