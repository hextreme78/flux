#include <kernel/uart-ns16550a.h>
#include <kernel/riscv64.h>
#include <kernel/irq.h>
#include <kernel/spinlock.h>
#include <kernel/kprintf.h>
#include <kernel/plic-sifive.h>

/* uart tx ring buffer */
static spinlock_t uart_tx_lock;
static volatile char uart_tx_ring[UART_TX_RING_SIZE];
static volatile size_t uart_tx_r = 0;
static volatile size_t uart_tx_w = 0;

/* uart rx ring buffer */
static spinlock_t uart_rx_lock;
static volatile char uart_rx_ring[UART_RX_RING_SIZE];
static volatile size_t uart_rx_r = 0;
static volatile size_t uart_rx_w = 0;

void uart_init(void)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;

	spinlock_init(&uart_tx_lock);
	spinlock_init(&uart_rx_lock);

	/* set DLAB bit in LCR to set baudrate divisor */
	base->lcr = UART_LCR_DLAB;
	base->dll = UART_DLL_DIVISOR;
	base->dlm = UART_DLM_DIVISOR;

	/* set word size for tx and rx */
	base->lcr = UART_LCR_DATA_NBITS_8;

	/* enable fifo in FCR */
	base->fcr = UART_FCR_FIFO;

	/* enable interrupts in MCR */
	base->mcr = UART_MCR_INTERRUPTS;

	/* enable interrupts in IER */
	base->ier = UART_IER_RDR_INTERRUPT | UART_IER_TBE_INTERRUPT;
}

void uart_tx_flush_async(void)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;
	spinlock_acquire(&uart_tx_lock);
	if (uart_tx_w != uart_tx_r) {
		/* Put one byte from tx ring into THR to
		 * raise TBE interrupt to flush tx ring.
		 */
		while (!(base->lsr & UART_LSR_TBE_MASK));
		base->thr = uart_tx_ring[uart_tx_r];
		uart_tx_r = (uart_tx_r + 1) % UART_TX_RING_SIZE;
	}
	spinlock_release(&uart_tx_lock);
}

void uart_tx_flush_sync(void)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;
	spinlock_acquire(&uart_tx_lock);
	while (uart_tx_w != uart_tx_r) {
		while (!(base->lsr & UART_LSR_TBE_MASK));
		for (size_t i = 0; i < UART_FIFO_SIZE; i++) {
			if (uart_tx_w == uart_tx_r) {
				break;
			}
			base->thr = uart_tx_ring[uart_tx_r];
			uart_tx_r = (uart_tx_r + 1) % UART_TX_RING_SIZE;
		}
	}
	spinlock_release(&uart_tx_lock);
}

void uart_rx_reset(void)
{
	spinlock_acquire(&uart_rx_lock);
	uart_rx_w = uart_rx_r = 0;
	spinlock_release(&uart_rx_lock);
}

void uart_putch_async(char ch)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;
	spinlock_acquire(&uart_tx_lock);
	if ((uart_tx_w + 1) % UART_TX_RING_SIZE == uart_tx_r) {
		/* Put one byte into thr if tx ring is full.
		 * Then put new char into tx ring.
		 */
		while (!(base->lsr & UART_LSR_TBE_MASK));
		base->thr = uart_tx_ring[uart_tx_r];
		uart_tx_r = (uart_tx_r + 1) % UART_TX_RING_SIZE;
	}
	uart_tx_ring[uart_tx_w] = ch;
	uart_tx_w = (uart_tx_w + 1) % UART_TX_RING_SIZE;
	spinlock_release(&uart_tx_lock);
}

char uart_getch_async(void)
{
	char ch;
	spinlock_acquire(&uart_rx_lock);
	while (uart_rx_r == uart_rx_w) {
		spinlock_release(&uart_rx_lock);
		wfi();
		spinlock_acquire(&uart_rx_lock);
	}
	ch = uart_rx_ring[uart_rx_r];
	uart_rx_r = (uart_rx_r + 1) % UART_RX_RING_SIZE;
	spinlock_release(&uart_rx_lock);
	return ch;
}

void uart_putch_sync(char ch)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;

	spinlock_acquire(&uart_tx_lock);
	while (!(base->lsr & UART_LSR_TBE_MASK));

	base->thr = ch;
	spinlock_release(&uart_tx_lock);
}

char uart_getch_sync(void)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;
	char ch;

	spinlock_acquire(&uart_rx_lock);
	while (!(base->lsr & UART_LSR_RDR_MASK));

	ch = base->rhr;
	spinlock_release(&uart_rx_lock);

	return ch;
}

void uart_irq_handler(void)
{
	uart_mmio_t *base = (uart_mmio_t *) VIRT_UART0;
	uint8_t isrcode = base->isr & UART_ISR_CODE_MASK;

	switch (isrcode) {
	case UART_ISR_RDR_TRIGGER_INTERRUPT:
	case UART_ISR_RDR_INTERRUPT:
		spinlock_acquire(&uart_rx_lock);
		while (base->lsr & UART_LSR_RDR_MASK) {
			if ((uart_rx_w + 1) % UART_RX_RING_SIZE == uart_rx_r) {
				/* overflow buffer */
				uart_rx_r = (uart_rx_r + 1) % UART_RX_RING_SIZE;
			}
			uart_rx_ring[uart_rx_w] = base->rhr;
			uart_rx_w = (uart_rx_w + 1) % UART_RX_RING_SIZE;
		}
		spinlock_release(&uart_rx_lock);
		break;

	case UART_ISR_TBE_INTERRUPT:
		spinlock_acquire(&uart_tx_lock);
		while (!(base->lsr & UART_LSR_TBE_MASK));
		for (size_t i = 0; i < UART_FIFO_SIZE && uart_tx_r != uart_tx_w; i++) {
			base->thr = uart_tx_ring[uart_tx_r];
			uart_tx_r = (uart_tx_r + 1) % UART_TX_RING_SIZE;
		}
		spinlock_release(&uart_tx_lock);
		break;

	default:
		panic("unknown uart interrupt");
	}
}

