#ifndef PCI_HELPER
#define PCI_HELPER

unsigned pci_inb(uint16_t port);
unsigned pci_inw(uint16_t port);
unsigned pci_inl(uint16_t port);

void pci_outb(uint16_t port, uint8_t value);
void pci_outw(uint16_t port, uint16_t value);
void pci_outl(uint16_t port, u32_t value);

#endif
