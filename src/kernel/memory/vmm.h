#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Page Directory Entry Flags
#define PDE_PRESENT 0x1
#define PDE_RW      0x2
#define PDE_USER    0x4

// Page Table Entry Flags
#define PTE_PRESENT 0x1
#define PTE_RW      0x2
#define PTE_USER    0x4

void init_vmm();
int vmm_map_page(void* phys_addr, void* virt_addr);
void* vmm_alloc_page(void* virt_addr);
void vmm_free_page(void* virt_addr);

#endif
