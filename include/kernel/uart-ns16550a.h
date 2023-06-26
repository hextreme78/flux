#ifndef KERNEL_UART_NS16550A_H
#define KERNEL_UART_NS16550A_H

#include <kernel/types.h>

#define UART_DL_DIVISOR 1
#define UART_DLL_DIVISOR UART_DL_DIVISOR & 0x00ff
#define UART_DLM_DIVISOR ((UART_DL_DIVISOR & 0xff00) >> 8)

#define UART_LCR_DATA_NBITS_8 3
#define UART_LCR_DLAB (1 << 7)

#define UART_LSR_RDR_MASK 1
#define UART_LSR_TBE_MASK (1 << 5)

#define UART_FCR_FIFO 1

#define UART_MCR_INTERRUPTS (1 << 3)

#define UART_IER_RDR_INTERRUPT 1
#define UART_IER_TBE_INTERRUPT (1 << 1)

#define UART_ISR_CODE_MASK 0xf
#define UART_ISR_RDR_TRIGGER_INTERRUPT (1 << 2)
#define UART_ISR_RDR_INTERRUPT (3 << 2)
#define UART_ISR_TBE_INTERRUPT (1 << 1)
#define UART_ISR_NO_INTERRUPT 1

#define UART_FIFO_SIZE 16
#define UART_TX_RING_SIZE 256
#define UART_RX_RING_SIZE 256

typedef volatile struct {
	union {
		u8 rhr;
		u8 thr;
		u8 dll;
	};
	union {
		u8 ier;
		u8 dlm;
	};
	union {
		u8 isr;
		u8 fcr;
	};
	u8 lcr;
	u8 mcr;
	union {
		u8 lsr;
		u8 psd;
	};
	u8 msr;
	u8 spr;
} __attribute__((packed)) uart_mmio_t;

void uart_init(void);

void uart_putch_async(char ch);
char uart_getch_async(void);
void uart_tx_flush_async(void);

void uart_putch_sync(char ch);
char uart_getch_sync(void);
void uart_tx_flush_sync(void);

void uart_rx_reset(void);

void uart_irq_handler(void);

#endif

