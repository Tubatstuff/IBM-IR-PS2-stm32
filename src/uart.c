#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include "globals.h"
#include "uart.h"

#define UART_TX_BUF_LEN 64		// transmit buffer length

static uint8_t
  UartTxBuf[ UART_TX_BUF_LEN];

static volatile int
  UartTxIn,
  UartTxOut;				// buffer pointers


// local prototypes.

static void Numout( unsigned Num, int Dig, int Radix, int bwz);
static void Uput( unsigned char What);

//*   InitUART - Set bitrate, data bits, etc.
//    ----------------------------------------
//
//    Called once on initialization.
//

void InitUART (int Baudrate)
{

//  Enable the GPIO pins and clocks.

  rcc_periph_clock_enable (RCC_USART1);

//  Set up the GPIO pins

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);
                      
  usart_set_baudrate( USART1, Baudrate);
  
// Setup USART1 parameters. 

  usart_set_databits (USART1, 8);
  usart_set_stopbits (USART1, USART_STOPBITS_1);
  usart_set_parity (USART1, USART_PARITY_NONE);
  usart_set_flow_control (USART1, USART_FLOWCONTROL_NONE);
  usart_set_mode (USART1, USART_MODE_TX_RX);

//  Set up interrupt on TxE.

  nvic_enable_irq(NVIC_USART1_IRQ);

// Finally enable the USART. 

  UartTxIn = UartTxOut = 0;	// clear buffer
  usart_enable (USART1);
  return;
} // InitUART

//  Ucharavail - Test if character ready.
//  -------------------------------------
//
//  Returns 0 if not ready, nonzero, otherwise
//

int Ucharavail( void)
{
  return (USART_SR(USART1) & USART_SR_RXNE) ? 1 : 0;
} // Ucharavail


//  Ugetchar - Get UART Character.
//  ------------------------------
//
//  Returns character when ready.
//

unsigned char Ugetchar( void)
{
  return (usart_recv_blocking( USART1) & 255);
} // Ugetchar


//  Uputchar - Put character out.
//  -----------------------------
//
//  Write character to UART.
//

void Uputchar( unsigned char What)
{

  if ( What == '\n')
    Uput( '\r');

  Uput( What);
  return;

} // UputChar


//  Put a character to output.
//  --------------------------
//
//  Queue the character and enable TX interrupt.
//

static void Uput( unsigned char What)
{

  int rNext;
  
  rNext = UartTxIn+1;
  if ( rNext >= UART_TX_BUF_LEN)
    rNext = 0;				// handle wrap
  if ( rNext != UartTxOut)
  { // if there's room, put it in
    UartTxBuf[UartTxIn] = What;		// stash the character
    UartTxIn = rNext;			// and advance the pointer
  }
  
//	Regardless, enable transmit interrupt.

  USART_CR1(USART1) |= USART_CR1_TXEIE;
  return;
} // Uput


//  Uputs - Put a string to UART.
//  -----------------------------
//
//  Ends with a null.  If a newline is present, adds a CR.
//

void Uputs( char *What)
{

  char
    c;

  while( (c = *What++))
    Uputchar( c);
} // Uputs

//  Usart_ISR - interrupt servicer.
//
//	Currently, transmit only.
//

void usart1_isr(void)
{

  if (((USART_CR1(USART1) & USART_CR1_TXEIE) != 0) &&
     ((USART_SR(USART1) & USART_SR_TXE) != 0)) 
  {
    if ( UartTxIn != UartTxOut)
    { // only if we have data
      usart_send( USART1, UartTxBuf[ UartTxOut++]);
      if (UartTxOut >= UART_TX_BUF_LEN)
        UartTxOut = 0;		// wrap
     }
     if ( UartTxIn != UartTxOut)
       USART_CR1(USART1) |= USART_CR1_TXEIE;   // ready for next character
     else
       USART_CR1(USART1) &= ~USART_CR1_TXEIE;  // disable if queue empty
  }  // if interrupt was from TX empty
} // usart1_isr


//* Uprintf - Simple printf function to UART.
//  -----------------------------------------
//
//    We understand %s and %nx, where n is either null or 1-8
//

void Uprintf( char *Form,...)
{

  const char *p;
  va_list argp;
  int width;
  int i, bwz;
  unsigned u;
  char *s;
  
  va_start(argp, Form);
  
  for ( p = Form; *p; p++)
  {  // go down the format string
    if ( *p != '%')
    { // if this is just a character, echo it
      Uputchar( *p);    
      continue;
    } // ordinary character
    
    ++p;                  // advance
    width = 0;            // assume no fixed width
    bwz = 1;              // say blank when zero
    
    if ( *p == '0')
    {
      p++;
      bwz = 0;            // don't blank when zero
    } // say don't suppress lead zeros
    
    while ( (*p >= '0') && (*p <= '9'))
      width = (width*10) + (*p++ - '0'); // get width of field

    switch (*p)
    {  

      case 'd':
        u = va_arg( argp, unsigned int);
        Numout( u, width, 10, bwz);    // output decimal string
        break;
              
      case 'x':
        u = va_arg( argp, unsigned int);
        Numout( u, width, 16, bwz);     // output hex string
        break;      

      case 'c':
        i = va_arg( argp, int);
        Uputchar(i);
        break;
        
      case 's':
        s = va_arg( argp, char *);
        
//  We have to handle lengths here, so eventually space-pad or truncate.

        if ( !width)
          Uputs(s);            // no width specified, just put it out
        else
        {  
          i = strlen(s) - width;
          if ( width >= 0)
            Uputs( s+i);       // truncate
          else
          {  // if pad
            for ( ; i; i++)
              Uputchar(' ');   // pad  
            Uputs(s);
          }
        } // we have a width specifier
        break;
      
      default:
        break;   
    } // switch
  } // for each character
  va_end( argp);
  return;
} // Uprintf

//  Ugets - Get a string.
//  ---------------------
//
//  Reads a string from input; ends with CR or LF on input
//  Strips the terminal CR or LF; terminal null appended.
//
//  Echo and backspace are also processed.
//
//  On input, address of string, length of string buffer.
//  Returns the address of the null.
//

char *Ugets( char *Buf, int Len)
{

  char c;
  int pos;
 
  for ( pos = 0; Len;)
  {
    c = Ugetchar();
    if ( c == '\r' || c == '\n')
      break;
    if ( c == '\b' || c == 127)
    {	// handle backspace.
      if (pos)
      {
      	Uputs( "\b \b");	// backspace-space-backspace
        pos--;
        Buf--;
        Len++;
      } // if not at the start of line
      continue;			// don't store anything
    } // if backspace
    if ( c >= ' ')
    {
      Uputchar( c);		// echo the character;
      *Buf++ = c;		// store character
      pos++;
      Len--;
      if ( !Len)
      	break;
    } // if printable character
  } // for
  *Buf = 0;
  Uputs( "\n");		// end with a newline
  return Buf;
} // Ugets


//  Numout - Output a number in any radix.
//  --------------------------------------
//
//  On entry, Num = number to display, Dig = digits, Radix = radix up to 16.
//  bwz = nonzero if blank when zero.
//  Nothing on return.
//

static void Numout( unsigned Num, int Dig, int Radix, int bwz)
{

  int i;
  const char hexDigit[] = "0123456789abcdef";
  char  outBuf[10];
  
  memset( outBuf, 0, sizeof outBuf);  // zero it all
  if (Radix  == 0 || Radix > 16)
    return;
  
// Use Chinese remainder theorem to develop conversion.

  i = (sizeof( outBuf) - 2); 
  do
  {  
    outBuf[i] = (char) (Num % Radix);
    Num = Num / Radix;
    i--;
   } while (Num);
   
//  If the number of digits is zero, just print the significant number.

  if ( Dig == 0)
    Dig = sizeof( outBuf) - 2 - i;
  for ( i = sizeof( outBuf) - Dig - 1; i < (int) sizeof( outBuf)-1; i++)
  {
    if ( bwz && !outBuf[i] && (i != (sizeof(outBuf)-2)) )
      Uputchar( ' ');
    else
    {
      bwz = 0;  
      Uputchar( hexDigit[ (int) outBuf[i]] );
    }  // not blanking lead zeroes
  }  
} // Numout


//  Hexin - Read a hex number until a non-hex.
//  ------------------------------------------
//
//    On exit, the hex value is stored in RetVal.
//    and the address of the next non-hex digit is returned.
//
//    If no valid digits are encountered, the return value is 0xffffffff;
//
//    Leading spaces are permitted.
//

char *Hexin( unsigned int *RetVal, unsigned int *Digits, char *Buf)
{

  unsigned int
    accum;
  char
    c;
  int
    digCount;

  digCount = 0;				// no digits yet
  accum = 0;				// set null accumulator

//  First, strip off any leading spaces.

  while( *Buf == ' ')
    Buf++;				// skip until non-space

//  Now, pick up digits.

  do
  {
    c = *Buf++;
    c = toupper(c);
    if ( c >= '0' && c <= '9')
      accum = (accum << 4) | (c - '0');
    else if (c >= 'A' && c <= 'F')
      accum = (accum << 4) | (c - 'A' +10);
    else
    {
      Buf--;
      *Digits = digCount;
      *RetVal = accum;
      return Buf;
    }
    digCount++;
  } while(1);
} // Hexin
