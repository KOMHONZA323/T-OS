#!/usr/bin/env python3
import sys
import struct
import os

def create_fat16_image(image_path, efi_file_path):
    # FAT16 Constants
    SECTOR_SIZE = 512
    SECTORS_PER_CLUSTER = 4 # 2KB clusters to keep count < 65525 for 64MB disk
    RESERVED_SECTORS = 1
    FAT_COUNT = 2
    ROOT_ENTRIES = 512
    TOTAL_SECTORS = 64 * 1024 * 1024 // SECTOR_SIZE # 64MB
    MEDIA_DESCRIPTOR = 0xF8

    # Calculate FAT sizes
    # Root dir size in sectors
    root_dir_sectors = (ROOT_ENTRIES * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE

    # FAT size calculation
    tmp_data_sectors = TOTAL_SECTORS - RESERVED_SECTORS - root_dir_sectors
    # Approximate fat size
    fat_size_sectors = (tmp_data_sectors * 2 + SECTOR_SIZE - 1) // SECTOR_SIZE

    # Refine calculation
    data_sectors = TOTAL_SECTORS - RESERVED_SECTORS - (FAT_COUNT * fat_size_sectors) - root_dir_sectors
    total_clusters = data_sectors // SECTORS_PER_CLUSTER

    # BPB Structure
    bpb = bytearray(SECTOR_SIZE)
    # Jump instruction
    bpb[0:3] = b'\xEB\x3C\x90'
    # OEM Name
    bpb[3:11] = b'T-OS    '
    # Bytes per sector
    struct.pack_into('<H', bpb, 11, SECTOR_SIZE)
    # Sectors per cluster
    bpb[13] = SECTORS_PER_CLUSTER
    # Reserved sectors
    struct.pack_into('<H', bpb, 14, RESERVED_SECTORS)
    # FAT copies
    bpb[16] = FAT_COUNT
    # Root entries
    struct.pack_into('<H', bpb, 17, ROOT_ENTRIES)
    # Small sectors
    if TOTAL_SECTORS < 65536:
        struct.pack_into('<H', bpb, 19, TOTAL_SECTORS)
    else:
        struct.pack_into('<H', bpb, 19, 0)
    # Media descriptor
    bpb[21] = MEDIA_DESCRIPTOR
    # Sectors per FAT
    struct.pack_into('<H', bpb, 22, fat_size_sectors)
    # Sectors per track
    struct.pack_into('<H', bpb, 24, 32)
    # Heads
    struct.pack_into('<H', bpb, 26, 64)
    # Hidden sectors
    struct.pack_into('<I', bpb, 28, 0)
    # Large sectors
    if TOTAL_SECTORS >= 65536:
        struct.pack_into('<I', bpb, 32, TOTAL_SECTORS)

    # EBPB (FAT12/16)
    bpb[36] = 0x80 # Drive number
    bpb[38] = 0x29 # Ext boot sig
    struct.pack_into('<I', bpb, 39, 0x12345678) # Serial
    bpb[43:54] = b'NO NAME    '
    bpb[54:62] = b'FAT16   '
    bpb[510:512] = b'\x55\xAA'

    # Create image file
    with open(image_path, 'wb') as f:
        # Write BPB
        f.write(bpb)

        # Write FATs (empty initially)
        fat_data = bytearray(fat_size_sectors * SECTOR_SIZE)
        # First 2 entries are reserved
        fat_data[0] = MEDIA_DESCRIPTOR
        fat_data[1] = 0xFF
        fat_data[2] = 0xFF
        fat_data[3] = 0xFF

        for _ in range(FAT_COUNT):
            f.write(fat_data)

        # Write Root Directory (empty initially)
        root_dir_data = bytearray(root_dir_sectors * SECTOR_SIZE)
        f.write(root_dir_data)

        # Write Data Area (empty)
        # We don't need to write all zeros, just seek to end to make file size correct
        f.seek(TOTAL_SECTORS * SECTOR_SIZE - 1)
        f.write(b'\x00')

    # Root Dir starts at sector: RESERVED_SECTORS + FAT_COUNT * fat_size_sectors
    root_dir_offset = (RESERVED_SECTORS + FAT_COUNT * fat_size_sectors) * SECTOR_SIZE
    data_offset = root_dir_offset + (root_dir_sectors * SECTOR_SIZE)

    with open(efi_file_path, 'rb') as src:
        efi_data = src.read()

    efi_size = len(efi_data)
    clusters_needed = (efi_size + (SECTORS_PER_CLUSTER * SECTOR_SIZE) - 1) // (SECTORS_PER_CLUSTER * SECTOR_SIZE)

    # Clusters: 2=EFI, 3=BOOT, 4..N=BOOTX64.EFI
    efi_dir_cluster = 2
    boot_dir_cluster = 3
    file_start_cluster = 4

    def get_cluster_offset(cluster):
        return data_offset + ((cluster - 2) * SECTORS_PER_CLUSTER * SECTOR_SIZE)

    # Open image for update
    with open(image_path, 'r+b') as f:
        def write_fat_entry(cluster, value):
             # FAT1 offset
             offset = SECTOR_SIZE * RESERVED_SECTORS + (cluster * 2)
             f.seek(offset)
             f.write(struct.pack('<H', value))
             # FAT2 offset
             offset2 = SECTOR_SIZE * RESERVED_SECTORS + (fat_size_sectors * SECTOR_SIZE) + (cluster * 2)
             f.seek(offset2)
             f.write(struct.pack('<H', value))

        # EFI Dir (Cluster 2) is End of Chain
        write_fat_entry(efi_dir_cluster, 0xFFFF)
        # BOOT Dir (Cluster 3) is End of Chain
        write_fat_entry(boot_dir_cluster, 0xFFFF)

        # File clusters
        for i in range(clusters_needed):
            current = file_start_cluster + i
            if i == clusters_needed - 1:
                next_cluster = 0xFFFF # EOF
            else:
                next_cluster = current + 1
            write_fat_entry(current, next_cluster)

        # Write Directory Entries

        # Root Directory Entry for EFI
        f.seek(root_dir_offset)
        # Name: "EFI        "
        f.write(b'EFI        ')
        # Attr: Directory (0x10)
        f.write(b'\x10')
        f.write(b'\x00') # Reserved
        f.write(b'\x00' * 5) # Time stuff
        f.write(b'\x00\x00') # Access date
        f.write(b'\x00\x00') # High cluster
        f.write(b'\x00\x00\x00\x00') # Write Time/Date
        f.write(struct.pack('<H', efi_dir_cluster))
        f.write(b'\x00\x00\x00\x00') # Size

        # EFI Directory Content (at Cluster 2)
        efi_dir_offset = get_cluster_offset(efi_dir_cluster)
        f.seek(efi_dir_offset)

        # . entry
        f.write(b'.          ')
        f.write(b'\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
        f.write(struct.pack('<H', efi_dir_cluster))
        f.write(b'\x00\x00\x00\x00')

        # .. entry (points to Root - cluster 0)
        f.write(b'..         ')
        f.write(b'\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
        f.write(b'\x00\x00')
        f.write(b'\x00\x00\x00\x00')

        # BOOT entry
        f.write(b'BOOT       ')
        f.write(b'\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
        f.write(struct.pack('<H', boot_dir_cluster))
        f.write(b'\x00\x00\x00\x00')

        # BOOT Directory Content (at Cluster 3)
        boot_dir_offset = get_cluster_offset(boot_dir_cluster)
        f.seek(boot_dir_offset)

        # . entry
        f.write(b'.          ')
        f.write(b'\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
        f.write(struct.pack('<H', boot_dir_cluster))
        f.write(b'\x00\x00\x00\x00')

        # .. entry (points to EFI - cluster 2)
        f.write(b'..         ')
        f.write(b'\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
        f.write(struct.pack('<H', efi_dir_cluster))
        f.write(b'\x00\x00\x00\x00')

        # BOOTX64.EFI entry
        # Name needs to be uppercase and space padded
        f.write(b'BOOTX64 EFI')
        f.write(b'\x20') # Archive
        f.write(b'\x00')
        f.write(b'\x00' * 5)
        f.write(b'\x00\x00')
        f.write(b'\x00\x00') # High cluster
        f.write(b'\x00\x00\x00\x00') # Time
        f.write(struct.pack('<H', file_start_cluster))
        f.write(struct.pack('<I', efi_size))

        # Write File Data
        file_offset = get_cluster_offset(file_start_cluster)
        f.seek(file_offset)
        f.write(efi_data)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: make_fat.py <image_path> <efi_file_path>")
        sys.exit(1)

    create_fat16_image(sys.argv[1], sys.argv[2])
