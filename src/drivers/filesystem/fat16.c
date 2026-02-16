#include "fat16.h"
#include "../ata.h"
#include "../utils.h"
#include "../../kernel/memory/heap.h"
#include "../screen.h"

// Partition Info
static uint32_t partition_lba_start = 0;
static uint32_t partition_sectors = 0;

// BPB Info
static uint16_t bytes_per_sector;
static uint8_t sectors_per_cluster;
static uint16_t reserved_sectors;
static uint8_t num_fats;
static uint16_t root_entries;
static uint16_t total_sectors;
static uint16_t sectors_per_fat;

// Calculated Offsets (LBA)
static uint32_t fat_start_lba;
static uint32_t root_dir_start_lba;
static uint32_t data_start_lba;

static int fs_ready = 0;

typedef struct {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t reserved;
    uint8_t ctime_th;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_high;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) FatEntry;

void init_fat16() {
    uint8_t buffer[512];
    memory_set((char*)buffer, 0, 512);

    if (!ata_read_sector(0, buffer)) {
        kprint("Disk Error: Read Failed or No Disk.\n");
        fs_ready = 0;
        return;
    }

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        kprint("Disk Warning: No Boot Signature (0x55AA). Skipping FS.\n");
        fs_ready = 0;
        return;
    }

    // Check for VBR signature (Superfloppy)
    // 0xEB/0xE9 at offset 0 (JMP) AND 0x29 at offset 38 (Extended Boot Sig)
    // Actually, offset 38 is 0x29 for FAT12/16.
    if ((buffer[0] == 0xEB || buffer[0] == 0xE9) && buffer[38] == 0x29) {
        // Superfloppy
        partition_lba_start = 0;
        // kprint("Mounting as Superfloppy (FAT16).\n");
    } else {
        // Assume MBR
        uint8_t* p1 = &buffer[446];
        partition_lba_start = *(uint32_t*)&p1[8];
        partition_sectors = *(uint32_t*)&p1[12];
        // kprint("Mounting from Partition 1.\n");

        if (partition_lba_start == 0) {
             // Fallback if MBR looks empty but we have signature
             // Check if it looks like BPB anyway?
             if (buffer[38] == 0x29) partition_lba_start = 0;
        }
    }

    // Read Boot Sector (if MBR, read partition start; if Superfloppy, read 0 again)
    if (!ata_read_sector(partition_lba_start, buffer)) {
        kprint("Disk Error: Failed to read Boot Sector.\n");
        fs_ready = 0;
        return;
    }

    bytes_per_sector = *(uint16_t*)&buffer[11];
    sectors_per_cluster = buffer[13];
    reserved_sectors = *(uint16_t*)&buffer[14];
    num_fats = buffer[16];
    root_entries = *(uint16_t*)&buffer[17];
    total_sectors = *(uint16_t*)&buffer[19];
    if (total_sectors == 0) total_sectors = *(uint32_t*)&buffer[32];
    sectors_per_fat = *(uint16_t*)&buffer[22];

    fat_start_lba = partition_lba_start + reserved_sectors;
    root_dir_start_lba = fat_start_lba + (num_fats * sectors_per_fat);
    data_start_lba = root_dir_start_lba + (root_entries * 32 / bytes_per_sector);

    fs_ready = 1;
}

// Convert Cluster to LBA
uint32_t cluster_to_lba(uint16_t cluster) {
    return data_start_lba + ((cluster - 2) * sectors_per_cluster);
}

// Read FAT Entry
uint16_t read_fat_entry(uint16_t cluster) {
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_lba + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    uint8_t buffer[512];
    if (!ata_read_sector(fat_sector, buffer)) return 0xFFFF;
    return *(uint16_t*)&buffer[ent_offset];
}

void to_dos_filename(const char* input, char* output) {
    memory_set(output, ' ', 11);
    int i = 0;
    int j = 0;
    while (input[i] && input[i] != '.' && j < 8) {
        char c = input[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        output[j++] = c;
    }
    while (input[i] && input[i] != '.') i++;
    if (input[i] == '.') i++;
    j = 8;
    while (input[i] && j < 11) {
        char c = input[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        output[j++] = c;
    }
}

int fat16_read_file(const char* filename, void* buffer) {
    if (!fs_ready) return 0;

    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint8_t sector[512];
    uint32_t root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector;

    for (uint32_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_dir_start_lba + i, sector)) return 0;
        FatEntry* entries = (FatEntry*)sector;
        for (int j = 0; j < bytes_per_sector / 32; j++) {
            if (entries[j].name[0] == 0) return 0;
            if (entries[j].name[0] == 0xE5) continue;

            int match = 1;
            for (int k = 0; k < 11; k++) {
                if (((uint8_t*)entries[j].name)[k] != dos_name[k]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                uint16_t cluster = entries[j].cluster_low;
                uint32_t size = entries[j].size;
                uint8_t* buf = (uint8_t*)buffer;
                uint32_t read_size = 0;

                while (read_size < size) {
                    uint32_t lba = cluster_to_lba(cluster);
                    for (int s = 0; s < sectors_per_cluster; s++) {
                        if (!ata_read_sector(lba + s, buf)) return 0;
                        buf += 512;
                        read_size += 512;
                        if (read_size >= size) break;
                    }
                    if (read_size >= size) break;

                    cluster = read_fat_entry(cluster);
                    if (cluster >= 0xFFF8) break;
                }
                return 1;
            }
        }
    }
    return 0;
}

void fat16_list_directory() {
    if (!fs_ready) {
        kprint("No Filesystem mounted.\n");
        return;
    }

    uint8_t sector[512];
    uint32_t root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector;

    kprint("Files:\n");
    for (uint32_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_dir_start_lba + i, sector)) {
            kprint("Read Error listing dir.\n");
            return;
        }
        FatEntry* entries = (FatEntry*)sector;
        for (int j = 0; j < bytes_per_sector / 32; j++) {
            if (entries[j].name[0] == 0) return;
            if (entries[j].name[0] == 0xE5) continue;
            if (entries[j].attr & 0x0F) continue;

            char name[13];
            int k = 0;
            for (int l = 0; l < 8 && entries[j].name[l] != ' '; l++) name[k++] = entries[j].name[l];
            name[k++] = '.';
            for (int l = 0; l < 3 && entries[j].ext[l] != ' '; l++) name[k++] = entries[j].ext[l];
            if (name[k-1] == '.') k--;
            name[k] = 0;

            kprint("  ");
            kprint(name);
            kprint("\n");
        }
    }
}

int fat16_create_file(const char* filename) {
    if (!fs_ready) return 0;

    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint8_t sector[512];
    uint32_t root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector;

    for (uint32_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_dir_start_lba + i, sector)) return 0;
        FatEntry* entries = (FatEntry*)sector;
        for (int j = 0; j < bytes_per_sector / 32; j++) {
            if (entries[j].name[0] == 0 || entries[j].name[0] == 0xE5) {
                memory_copy(dos_name, (char*)entries[j].name, 11);
                entries[j].attr = 0x20;
                entries[j].cluster_low = 0;
                entries[j].size = 0;
                ata_write_sector(root_dir_start_lba + i, sector);
                return 1;
            }
        }
    }
    return 0;
}

// Helper to convert FAT 8.3 to string "NAME.EXT"
void fat_to_string(char* name83, char* out) {
    int k = 0;
    for (int l = 0; l < 8 && name83[l] != ' '; l++) out[k++] = name83[l];
    if (name83[8] != ' ') {
        out[k++] = '.';
        for (int l = 8; l < 11 && name83[l] != ' '; l++) out[k++] = name83[l];
    }
    out[k] = 0;
}

// Case insensitive prefix check
int starts_with_ignore_case(char* str, char* prefix) {
    while (*prefix) {
        char c1 = *str;
        char c2 = *prefix;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return 0;
        str++;
        prefix++;
    }
    return 1;
}

int fat16_find_file(char* partial, char* output) {
    if (!fs_ready) return 0;

    uint8_t sector[512];
    uint32_t root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector;

    for (uint32_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_dir_start_lba + i, sector)) return 0;
        FatEntry* entries = (FatEntry*)sector;
        for (int j = 0; j < bytes_per_sector / 32; j++) {
            if (entries[j].name[0] == 0) return 0;
            if (entries[j].name[0] == 0xE5) continue;
            if (entries[j].attr & 0x0F) continue;

            char fname[13];
            // Reconstruct name from 8.3 entry
            // Need to combine Name (8) and Ext (3) which are adjacent in struct?
            // Struct: uint8_t name[8]; uint8_t ext[3];
            // In memory they are contiguous 11 bytes.
            // My struct definition has them separate.
            // fat_to_string expects 11 bytes.
            // I can pass entries[j].name assuming packing.
            fat_to_string((char*)entries[j].name, fname);

            if (starts_with_ignore_case(fname, partial)) {
                // Found match!
                // Copy to output
                int m=0;
                while (fname[m]) { output[m] = fname[m]; m++; }
                output[m] = 0;
                return 1;
            }
        }
    }
    return 0;
}

void fat16_list_matches(char* partial) {
    if (!fs_ready) return;

    uint8_t sector[512];
    uint32_t root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector;

    int matches_found = 0;

    // We can't print directly to terminal from driver easily unless we use kprint.
    // kprint goes to screen. Terminal history is in kernel.c.
    // If we kprint, it might overlay on GUI.
    // But  uses kprint.
    // So CMakeLists.txt
README.md
VM.txt
src
temp_boot.asm output goes to debug log / screen overlay?
    // The user sees CMakeLists.txt
README.md
VM.txt
src
temp_boot.asm output?
    // User said "ls doesnt work". I fixed it.
    // If CMakeLists.txt
README.md
VM.txt
src
temp_boot.asm works, it means  works.
    // So I will use .

    for (uint32_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_dir_start_lba + i, sector)) return;
        FatEntry* entries = (FatEntry*)sector;
        for (int j = 0; j < bytes_per_sector / 32; j++) {
            if (entries[j].name[0] == 0) return;
            if (entries[j].name[0] == 0xE5) continue;
            if (entries[j].attr & 0x0F) continue;

            char fname[13];
            fat_to_string((char*)entries[j].name, fname);

            if (starts_with_ignore_case(fname, partial)) {
                kprint(fname);
                kprint(" ");
                matches_found++;
            }
        }
    }
    if (matches_found > 0) kprint("\n");
}
