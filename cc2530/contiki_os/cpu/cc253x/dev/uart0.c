/**
 * \file
 *
 *   uart0 write routines
 *
 * \author
 *
 *   Anthony "Asterisk" Ambuehl
 *
 */
#include <stdlib.h>
#include <string.h>

#include "cc253x.h"
#include "sfr-bits.h"
#include "dev/uart0.h"

#if UART0_ENABLE
/*---------------------------------------------------------------------------*/
void
uart0_init()
{
#if UART0_CONF_HIGH_SPEED
  UART_SET_SPEED(0, UART_460_M, UART_460_E);
#else
  UART_SET_SPEED(0, UART_9_M, UART_9_E);
#endif

#ifdef UART0_ALTERNATIVE_2
  PERCFG |= PERCFG_U0CFG;  / *alternative port 2 = P1.5-2 */
#ifdef UART0_RTSCTS
  P1SEL |= 0x3C;    /* peripheral select for TX and RX, RTS, CTS */
  P1DIR |= 0x08;    /* RTS out */
  P1DIR &= ~0x04;   /* CTS in */
#else
  P1SEL |= 0x30;    /* peripheral select for TX and RX */
  P1DIR |= 0x20;    /* TX out */
  P1DIR &= ~0x10;   /* RX in */
#endif
#else
  PERCFG &= ~PERCFG_U0CFG; /* alternative port 1 = P0.5-2 */
#ifdef UART0_RTSCTS
  P0SEL |= 0x3C;    /* peripheral select for TX and RX, RTS, CTS */
  P0DIR |= 0x20;    /* RTS out */
  P0DIR &= ~0x10;   /* CTS in */
#else
  P0SEL |= 0x0C;    /* peripheral select for TX and RX */
  P0DIR |= 0x08;    /* TX out */
  P0DIR &= ~0x04;   /* RX in */
#endif
#endif


#ifdef UART0_RTSCTS
  U0UCR = 0x42; /*defaults: 8N1, RTS/CTS, high stop bit*/
#else
  U0UCR = 0x02; /*defaults: 8N1, no flow control, high stop bit*/
#endif

  U0CSR = UCSR_MODE; /* UART mode */
  U0UCR |= 0x80; /* Flush */
  UART0_RX_EN();

  UART0_RX_INT(1);
}
/*---------------------------------------------------------------------------*/
/* Write one byte over the UART. */
void
uart0_writeb(uint8_t byte)
{
  UTX0IF = 0;
  U0DBUF = byte;
  while(!UTX0IF); /* Wait until byte has been transmitted. */
  UTX0IF = 0;
}
#endif
