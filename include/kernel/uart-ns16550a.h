#ifndef KERNEL_UART_NS16550A_H
#define KERNEL_UART_NS16550A_H

#include <stdint.h>
#include <stddef.h>

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
		uint8_t rhr;
		uint8_t thr;
		uint8_t dll;
	};
	union {
		uint8_t ier;
		uint8_t dlm;
	};
	union {
		uint8_t isr;
		uint8_t fcr;
	};
	uint8_t lcr;
	uint8_t mcr;
	union {
		uint8_t lsr;
		uint8_t psd;
	};
	uint8_t msr;
	uint8_t spr;
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

