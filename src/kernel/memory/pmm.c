#include "pmm.h"
#include "../../drivers/utils.h"
#include "../../drivers/screen.h" // For debugging print

// Structure of E820 memory map entry
typedef struct {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

// Bitmap location: 2MB mark
#define PMM_BITMAP_ADDR 0x200000
#define PMM_BITMAP_SIZE (1024 * 1024 / 8) // 128KB covers 4GB

uint8_t* pmm_bitmap = (uint8_t*)PMM_BITMAP_ADDR;
uint32_t pmm_total_blocks = 0;
uint32_t pmm_used_blocks = 0;
uint32_t pmm_memory_size = 0;

void pmm_set_bit(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

void pmm_unset_bit(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int pmm_test_bit(uint32_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

void init_pmm() {
    uint16_t count = *(uint16_t*)MEMORY_MAP_COUNT_ADDR;
    e820_entry_t* map = (e820_entry_t*)MEMORY_MAP_ENTRY_ADDR;

    // 1. Determine total memory size
    for (int i = 0; i < count; i++) {
        if (map[i].type == 1) { // Usable RAM
            uint32_t top = map[i].base_low + map[i].length_low;
            if (top > pmm_memory_size) {
                pmm_memory_size = top;
            }
        }
    }

    pmm_total_blocks = pmm_memory_size / PMM_BLOCK_SIZE;

    // 2. Initialize bitmap to all used (1)
    memory_set((char*)pmm_bitmap, 0xFF, PMM_BITMAP_SIZE);
    pmm_used_blocks = pmm_total_blocks; // Initially all used

    // 3. Mark usable regions as free (0)
    for (int i = 0; i < count; i++) {
        if (map[i].type == 1) { // Usable
            uint32_t start_block = map[i].base_low / PMM_BLOCK_SIZE;
            uint32_t num_blocks = map[i].length_low / PMM_BLOCK_SIZE;

            for (uint32_t j = 0; j < num_blocks; j++) {
                if (pmm_test_bit(start_block + j)) {
                    pmm_unset_bit(start_block + j);
                    pmm_used_blocks--;
                }
            }
        }
    }

    // 4. Mark kernel and reserved area as used
    // 0 - 4MB (Kernel, Bitmap, etc)
    // 4MB = 1024 blocks
    for (int i = 0; i < 1024; i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            pmm_used_blocks++;
        }
    }
}

void* pmm_alloc_page() {
    for (uint32_t i = 0; i < pmm_total_blocks; i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            pmm_used_blocks++;
            return (void*)(i * PMM_BLOCK_SIZE);
        }
    }
    return 0; // Out of memory
}

void pmm_free_page(void* addr) {
    uint32_t block = (uint32_t)addr / PMM_BLOCK_SIZE;
    if (pmm_test_bit(block)) {
        pmm_unset_bit(block);
        pmm_used_blocks--;
    }
}

uint32_t pmm_get_total_memory() {
    return pmm_memory_size;
}

uint32_t pmm_get_used_memory() {
    return pmm_used_blocks * PMM_BLOCK_SIZE;
}
