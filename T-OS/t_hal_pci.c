#include "t_hal_pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_find_gpu(uint8_t* out_bus, uint8_t* out_slot) {
    *out_bus = 0xFF;
    *out_slot = 0xFF;
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor = pci_read_dword(bus, slot, 0, 0) & 0xFFFF;
            if (vendor == 0xFFFF) continue; // Device doesn't exist
            
            uint32_t class_subclass = pci_read_dword(bus, slot, 0, 0x08) >> 16;
            if (class_subclass == 0x0300) { // VGA Display Controller
                *out_bus = bus;
                *out_slot = slot;
                return;
            }
        }
    }
}
