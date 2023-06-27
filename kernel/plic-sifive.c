#include <kernel/plic-sifive.h>

void plic_init(void)
{
	/* set source priority for irq */
	/* priority should be greater than treshold */
	plic_source_priority(VIRT_PLIC_UART0, 1);

	plic_source_priority(0x01, 1); //virtio
	plic_source_priority(0x02, 1); //virtio
	plic_source_priority(0x03, 1); //virtio
	plic_source_priority(0x04, 1); //virtio
	plic_source_priority(0x05, 1); //virtio
	plic_source_priority(0x06, 1); //virtio
	plic_source_priority(0x07, 1); //virtio
	plic_source_priority(0x08, 1); //virtio
}

void plic_hart_init(void)
{
	/* set plic treshold for hart */
	plic_priority_treshold(0);

	/* enable irq lines for hart */
	plic_irq_on(VIRT_PLIC_UART0);
	plic_irq_on(VIRT_PLIC_VIRTIO0);
	plic_irq_on(VIRT_PLIC_VIRTIO1);
	plic_irq_on(VIRT_PLIC_VIRTIO2);
	plic_irq_on(VIRT_PLIC_VIRTIO3);
	plic_irq_on(VIRT_PLIC_VIRTIO4);
	plic_irq_on(VIRT_PLIC_VIRTIO5);
	plic_irq_on(VIRT_PLIC_VIRTIO6);
	plic_irq_on(VIRT_PLIC_VIRTIO7);
}

