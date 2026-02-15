#include "ata.h"
#include "ports.h"

// Wait for BSY to be 0 and RDY to be 1
// Returns 1 on success, 0 on timeout (no drive) or error
int ata_wait_ready() {
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = port_byte_in(0x1F7);
        if (status == 0xFF) return 0; // Floating bus (no drive)
        if (status & 0x01) return 0; // ERR bit set
        if (status & 0x20) return 0; // DF (Device Fault) bit set
        if ((status & 0xC0) == 0x40) return 1; // BSY=0, RDY=1
    }
    return 0; // Timeout
}

int ata_read_sector(uint32_t lba, uint8_t *buffer) {
    if (!ata_wait_ready()) return 0; // No drive or busy

    port_byte_out(0x1F2, 1); // Sector Count
    port_byte_out(0x1F3, (uint8_t)lba);
    port_byte_out(0x1F4, (uint8_t)(lba >> 8));
    port_byte_out(0x1F5, (uint8_t)(lba >> 16));
    // 0xE0 (Master) | (lba >> 24)
    port_byte_out(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    port_byte_out(0x1F7, 0x20); // Read Command

    // Wait for DRQ
    uint32_t timeout = 100000;
    while (!(port_byte_in(0x1F7) & 0x08)) {
        uint8_t status = port_byte_in(0x1F7);
        if (status & 0x01) return 0; // Error
        if (status & 0x20) return 0; // Fault
        if (--timeout == 0) return 0;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t data = port_word_in(0x1F0);
        buffer[i*2] = (uint8_t)data;
        buffer[i*2+1] = (uint8_t)(data >> 8);
    }
    return 1;
}

int ata_write_sector(uint32_t lba, uint8_t *buffer) {
    if (!ata_wait_ready()) return 0;

    port_byte_out(0x1F2, 1);
    port_byte_out(0x1F3, (uint8_t)lba);
    port_byte_out(0x1F4, (uint8_t)(lba >> 8));
    port_byte_out(0x1F5, (uint8_t)(lba >> 16));
    port_byte_out(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    port_byte_out(0x1F7, 0x30); // Write Command

    // Wait for DRQ
    uint32_t timeout = 100000;
    while (!(port_byte_in(0x1F7) & 0x08)) {
        uint8_t status = port_byte_in(0x1F7);
        if (status & 0x01) return 0;
        if (status & 0x20) return 0;
        if (--timeout == 0) return 0;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        port_word_out(0x1F0, data);
    }

    // Flush Cache
    port_byte_out(0x1F7, 0xE7);
    return ata_wait_ready();
}
