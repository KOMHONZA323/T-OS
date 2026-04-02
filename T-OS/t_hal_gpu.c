#include "t_hal_gpu.h"
#include "t_hal_pci.h"

extern void *kmalloc(uint64_t size);

typedef struct {
    uint32_t opcode;
    uint32_t size;
    uint64_t physical_payload_addr;
} GPU_Command;

#define RING_BUFFER_SIZE 1024
typedef struct {
    GPU_Command queue[RING_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
} GPU_RingBuffer;

GPU_RingBuffer* gpu_ring;
uint64_t gpu_mmio_base;

#define GPU_DOORBELL_OFFSET 0x40 

void gpu_init(uint8_t bus, uint8_t slot) {
    if (bus == 0xFF) return; // No GPU
    
    // Read BAR0 (MMIO Base)
    uint32_t bar0 = pci_read_dword(bus, slot, 0, 0x10);
    gpu_mmio_base = bar0 & 0xFFFFFFF0; // Mask out flags
    
    // Allocate contiguous physical memory for the ring buffer
    gpu_ring = (GPU_RingBuffer*)kmalloc(sizeof(GPU_RingBuffer));
    if (!gpu_ring) return;
    gpu_ring->head = 0;
    gpu_ring->tail = 0;
    
    // In a real environment, we'd map this via MMU. We use physical addressing here.
    if (gpu_mmio_base && gpu_mmio_base != 0xFFFFFFF0) {
        uint32_t* mmio = (uint32_t*)gpu_mmio_base;
        // Commented out to prevent crashes in unconfigured QEMU virtio devices
        // mmio[0x20 / 4] = (uint32_t)((uint64_t)gpu_ring & 0xFFFFFFFF);         // Addr Low
        // mmio[0x24 / 4] = (uint32_t)(((uint64_t)gpu_ring >> 32) & 0xFFFFFFFF); // Addr High
    }
}

void gpu_submit_command(uint32_t opcode, void* payload, uint32_t size) {
    if (!gpu_ring) return;
    
    uint32_t next_head = (gpu_ring->head + 1) % RING_BUFFER_SIZE;
    if (next_head == gpu_ring->tail) return; // Queue full

    gpu_ring->queue[gpu_ring->head].opcode = opcode;
    gpu_ring->queue[gpu_ring->head].size = size;
    gpu_ring->queue[gpu_ring->head].physical_payload_addr = (uint64_t)payload;
    
    gpu_ring->head = next_head;

    // Ring the Doorbell
    if (gpu_mmio_base && gpu_mmio_base != 0xFFFFFFF0) {
        uint32_t* mmio = (uint32_t*)gpu_mmio_base;
        // Commented out to prevent page fault if MMIO region is not identity-mapped 
        // or properly configured by EFI firmware for virtio-gpu.
        // mmio[GPU_DOORBELL_OFFSET / 4] = 1;
    }
}
