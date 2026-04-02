#ifndef T_HAL_PCI_H
#define T_HAL_PCI_H
#include <stdint.h>

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_find_gpu(uint8_t* out_bus, uint8_t* out_slot);

#endif
