#include "ata.h"
#include "ports.h"

// Wait for BSY to be 0 and RDY to be 1
void ata_wait_ready() {
    while ((port_byte_in(0x1F7) & 0xC0) != 0x40);
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_ready();

    port_byte_out(0x1F2, 1); // Sector Count
    port_byte_out(0x1F3, (uint8_t)lba);
    port_byte_out(0x1F4, (uint8_t)(lba >> 8));
    port_byte_out(0x1F5, (uint8_t)(lba >> 16));
    // 0xE0 (Master) | (lba >> 24)
    port_byte_out(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    port_byte_out(0x1F7, 0x20); // Read Command

    // Wait for DRQ
    while (!(port_byte_in(0x1F7) & 0x08));

    for (int i = 0; i < 256; i++) {
        uint16_t data = port_word_in(0x1F0);
        buffer[i*2] = (uint8_t)data;
        buffer[i*2+1] = (uint8_t)(data >> 8);
    }
}

void ata_write_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_ready();

    port_byte_out(0x1F2, 1);
    port_byte_out(0x1F3, (uint8_t)lba);
    port_byte_out(0x1F4, (uint8_t)(lba >> 8));
    port_byte_out(0x1F5, (uint8_t)(lba >> 16));
    port_byte_out(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    port_byte_out(0x1F7, 0x30); // Write Command

    // Wait for DRQ
    while (!(port_byte_in(0x1F7) & 0x08));

    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        port_word_out(0x1F0, data);
    }

    // Flush Cache
    port_byte_out(0x1F7, 0xE7);
    ata_wait_ready();
}
