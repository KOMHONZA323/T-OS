#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void ata_write_sector(uint32_t lba, uint8_t *buffer);
void ata_read_sector(uint32_t lba, uint8_t *buffer); // Helpful for debugging or reading config back

#endif
