#include "vmm.h"
#include "pmm.h"
#include "../../drivers/utils.h"
#include "../../drivers/screen.h" // For ScreenInfo definition

// Page Directory (must be aligned to 4KB)
uint32_t* page_directory = 0;

void vmm_enable_paging(uint32_t* directory) {
    asm volatile("mov %0, %%cr3" :: "r"(directory));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

int vmm_map_page(void* phys_addr, void* virt_addr) {
    uint32_t pd_index = (uint32_t)virt_addr >> 22;
    uint32_t pt_index = ((uint32_t)virt_addr >> 12) & 0x03FF;

    uint32_t* pd = page_directory;

    // Check if Page Table exists
    if (!(pd[pd_index] & PDE_PRESENT)) {
        uint32_t* pt = (uint32_t*)pmm_alloc_page();
        if (!pt) return 0; // OOM

        memory_set((char*)pt, 0, 4096);

        pd[pd_index] = (uint32_t)pt | PDE_PRESENT | PDE_RW;
    }

    uint32_t* pt = (uint32_t*)(pd[pd_index] & ~0xFFF);
    pt[pt_index] = (uint32_t)phys_addr | PTE_PRESENT | PTE_RW;

    return 1;
}

void init_vmm() {
    // 1. Allocate Page Directory
    page_directory = (uint32_t*)pmm_alloc_page();
    memory_set((char*)page_directory, 0, 4096);

    // 2. Identity Map 0-16MB (Kernel, etc)
    // 16MB = 4096 pages
    for (uint32_t i = 0; i < 4096; i++) {
        vmm_map_page((void*)(i * 4096), (void*)(i * 4096));
    }

    // 3. Map VBE Framebuffer
    ScreenInfo* screen_info = (ScreenInfo*)0x5000;
    if (screen_info->framebuffer != 0) {
        uint32_t fb_addr = screen_info->framebuffer;
        uint32_t fb_size = screen_info->width * screen_info->height * (screen_info->bpp / 8);
        // Align size to page boundary (round up)
        uint32_t pages = (fb_size + 4095) / 4096;

        for (uint32_t i = 0; i < pages; i++) {
            void* addr = (void*)(fb_addr + i * 4096);
            vmm_map_page(addr, addr);
        }
    }

    // 3. Enable Paging
    vmm_enable_paging(page_directory);
}

void* vmm_alloc_page(void* virt_addr) {
    void* phys = pmm_alloc_page();
    if (!phys) return 0;

    if (!vmm_map_page(phys, virt_addr)) {
        pmm_free_page(phys);
        return 0;
    }
    return phys;
}

void vmm_free_page(void* virt_addr) {
    uint32_t pd_index = (uint32_t)virt_addr >> 22;
    uint32_t pt_index = ((uint32_t)virt_addr >> 12) & 0x03FF;

    uint32_t* pd = page_directory;
    if (pd[pd_index] & PDE_PRESENT) {
        uint32_t* pt = (uint32_t*)(pd[pd_index] & ~0xFFF);
        if (pt[pt_index] & PTE_PRESENT) {
            void* phys = (void*)(pt[pt_index] & ~0xFFF);
            pmm_free_page(phys);
            pt[pt_index] = 0;
            // TLB flush needed? Yes.
            asm volatile("invlpg (%0)" :: "r"(virt_addr));
        }
    }
}
