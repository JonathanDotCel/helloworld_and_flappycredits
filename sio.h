#pragma once
//
// Hardware Registers
//

#define SIO_DATA 0xBf801050
#define pSIODATA *(volatile char *)SIO_DATA

#define SIO_STATUS 0xBf801054
#define pSIOSTATUS *(volatile char *)SIO_STATUS

#define SIO_MODE 0xBF801058
#define pSIOMODE *(volatile unsigned short *)SIO_MODE

#define SIO_CONTROL 0xBf80105A
#define pSIOCONTROL *(volatile unsigned short *)SIO_CONTROL

#define SIO_BAUD 0xBf80105E
#define pSIOBAUD *(volatile unsigned short *)SIO_BAUD

//#define SIO_I_STAT 0xBF801070
//#define SIO_I_MASK 0xBF801074

//#define SIO_NOINTERRUPTS 0
//#define SIO_USEINTERRUPTS 1

#pragma region(LibSIO format)

/* status bits */
#define SR_IRQ 0x200
#define SR_CTS 0x100
#define SR_DSR 0x80
#define SR_FE 0x20
#define SR_OE 0x10
#define SR_PERROR 0x8
#define SR_TXU 0x4
#define SR_RXRDY 0x2
#define SR_TXRDY 0x1

#define SIO_CTS 0x100
#define SIO_DSR 0x80
#define SIO_FE 0x20
#define SIO_OE 0x10
#define SIO_PERROR 0x8
#define SIO_TXU 0x4
#define SIO_RXRDY 0x2
#define SIO_TXRDY 0x1

/* control bits */
#define CR_DSRIEN 0x1000
#define CR_RXIEN 0x800
#define CR_TXIEN 0x400
#define CR_BUFSZ_1 0x0
#define CR_BUFSZ_2 0x100
#define CR_BUFSZ_4 0x200
#define CR_BUFSZ_8 0x300
#define CR_INTRST 0x40
#define CR_RTS 0x20
#define CR_ERRRST 0x10  // ack
#define CR_BRK 0x8
#define CR_RXEN 0x4
#define CR_DTR 0x2
#define CR_TXEN 0x1

#define SIO_BIT_DTR CR_DTR
#define SIO_BIT_RTS CR_RTS

/* mode bits */
#define MR_SB_00 0x0
#define MR_SB_01 0x40
#define MR_SB_10 0x80
#define MR_SB_11 0xC0
#define MR_P_EVEN 0x20
#define MR_PEN 0x10
#define MR_CHLEN_5 0x0
#define MR_CHLEN_6 0x4
#define MR_CHLEN_7 0x8
#define MR_CHLEN_8 0xC
#define MR_BR_1 0x1
#define MR_BR_16 0x2
#define MR_BR_64 0x3

#pragma endregion(LibSIO format)

