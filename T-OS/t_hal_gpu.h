#ifndef T_HAL_GPU_H
#define T_HAL_GPU_H

#include <stdint.h>

void gpu_init(uint8_t bus, uint8_t slot);
void gpu_submit_command(uint32_t opcode, void* payload, uint32_t size);

#endif
