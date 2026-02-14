#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PMM_BLOCK_SIZE 4096

// Address where memory map is stored by stage2
#define MEMORY_MAP_COUNT_ADDR 0x4000
#define MEMORY_MAP_ENTRY_ADDR 0x4004

void init_pmm();
void* pmm_alloc_page();
void pmm_free_page(void* addr);
uint32_t pmm_get_total_memory();
uint32_t pmm_get_used_memory();

#endif
