#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "../../drivers/utils.h"

#define HEAP_START 0xC0000000
#define PAGE_SIZE 4096

typedef struct Header {
    struct Header* next;
    size_t size;
    uint8_t free;
} Header;

static void* heap_end = (void*)HEAP_START;
static Header* first_block = NULL;

void* sbrk(int increment) {
    void* current_end = heap_end;
    void* new_end = (void*)((char*)heap_end + increment);

    // If expanding heap
    if (increment > 0) {
        // Calculate needed pages
        uint32_t current_page_limit = ((uint32_t)heap_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        uint32_t new_page_limit = ((uint32_t)new_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        // If current heap end is exactly on page boundary, start allocating from there.
        // But the loop condition addr < new_page_limit handles it.
        // If heap_end is 0x...000, current_page_limit is 0x...000.
        // If new_end is 0x...100, new_page_limit is 0x...1000.
        // Loop runs for addr = 0x...000. Correct.

        for (uint32_t addr = current_page_limit; addr < new_page_limit; addr += PAGE_SIZE) {
            void* phys = pmm_alloc_page();
            if (!phys) return (void*)-1; // OOM
            vmm_map_page(phys, (void*)addr);
        }
    }

    heap_end = new_end;
    return current_end;
}

void init_heap() {
    // Initial expansion
    sbrk(PAGE_SIZE * 16); // Start with 64KB heap

    first_block = (Header*)HEAP_START;
    first_block->next = NULL;
    first_block->size = (PAGE_SIZE * 16) - sizeof(Header);
    first_block->free = 1;
}

void* malloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 4 bytes
    size = (size + 3) & ~3;

    Header* current = first_block;
    while (current) {
        if (current->free && current->size >= size) {
            // Split block if possible
            if (current->size > size + sizeof(Header) + 4) {
                Header* new_block = (Header*)((char*)current + sizeof(Header) + size);
                new_block->size = current->size - size - sizeof(Header);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return (void*)(current + 1);
        }

        if (current->next == NULL) {
            // Expand heap
            void* new_mem = sbrk(PAGE_SIZE * 16); // Expand by 64KB
            if (new_mem == (void*)-1) return NULL;

            Header* new_block = (Header*)new_mem;
            new_block->size = (PAGE_SIZE * 16) - sizeof(Header);
            new_block->free = 1;
            new_block->next = NULL;

            current->next = new_block;
            // Continue loop to use this new block
        }

        current = current->next;
    }
    return NULL;
}

void free(void* ptr) {
    if (!ptr) return;

    Header* block = (Header*)ptr - 1;
    block->free = 1;

    // Merge with next if free
    if (block->next && block->next->free) {
        block->size += sizeof(Header) + block->next->size;
        block->next = block->next->next;
    }

    // Merge with previous?
    Header* current = first_block;
    while (current) {
        if (current->next == block && current->free) {
            current->size += sizeof(Header) + block->size;
            current->next = block->next;
            // No break, try to merge further? No, we just merged current and block.
            // Block is now part of current.
            // We should check if current can merge with ITS next (which was block's next).
            // But block's next was already merged into block in previous step.
            // So current->next points to what follows block.
            break;
        }
        current = current->next;
    }
}
