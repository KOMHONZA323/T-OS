#include "tester.h"
#include "../drivers/screen.h"
#include "../drivers/utils.h"
#include "memory/pmm.h"
#include "../drivers/filesystem/fat16.h"
#include "task/process.h"

void check_task() {
    int color = 0;
    while(1) {
        // Simple visual indicator of multitasking
        draw_rect_px(g_width - 20, g_height - 20, 10, 10, 0xFF000000 | (color++ * 100));
        for(volatile int i=0; i<1000000; i++);
    }
}

void run_system_checks() {
    clear_screen();
    kprint("Running System Checks...\n\n");
    swap_buffers();

    // 1. Memory Check
    uint32_t mem = pmm_get_total_memory();
    char buf[32];
    kprint("[OK] Memory Detected: ");
    int_to_ascii(mem / (1024*1024), buf);
    kprint(buf);
    kprint(" MB\n");
    swap_buffers();
    delay(500);

    // 2. Resolution Check
    kprint("[OK] Resolution: ");
    int_to_ascii(g_width, buf);
    kprint(buf);
    kprint("x");
    int_to_ascii(g_height, buf);
    kprint(buf);
    kprint("\n");
    swap_buffers();
    delay(500);

    // 3. Disk Check
    kprint("[..] Checking Disk (FAT16)...\n");
    swap_buffers();
    init_fat16();
    fat16_list_directory();
    kprint("[OK] Disk Check Complete.\n");
    swap_buffers();
    delay(500);

    // 4. Multitasking Check
    kprint("[..] Creating Test Task...\n");
    create_process(check_task);
    kprint("[OK] Task Created (Check bottom right corner).\n");
    swap_buffers();

    kprint("\nSystem Checks Complete.\n");
    swap_buffers();

    // Simple delay to let user see results
    delay(3000);
}
