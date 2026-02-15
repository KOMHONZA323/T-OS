#ifndef ATA_H
#define ATA_H

#include <stdint.h>

int ata_write_sector(uint32_t lba, uint8_t *buffer);
int ata_read_sector(uint32_t lba, uint8_t *buffer); // Helpful for debugging or reading config back

#endif
