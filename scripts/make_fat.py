#!/usr/bin/env python3
import sys
import struct
import os
import shutil

def create_fat16_volume(volume_path, efi_file_path):
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
    with open(volume_path, 'wb') as f:
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
    with open(volume_path, 'r+b') as f:
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

    return TOTAL_SECTORS

def create_partitioned_image(image_path, efi_file_path):
    # 1. Create FAT Volume in a temp file
    temp_fat = image_path + ".fat"
    fat_sectors = create_fat16_volume(temp_fat, efi_file_path)

    # 2. Create Final Disk Image with MBR
    # Layout:
    # Sector 0: MBR
    # Sector 1..2047: Padding
    # Sector 2048..End: FAT Volume

    start_lba = 2048
    disk_sectors = start_lba + fat_sectors
    sector_size = 512

    with open(image_path, 'wb') as f:
        # Create MBR
        mbr = bytearray(sector_size)

        # Partition Entry 1 (Offset 446 / 0x1BE)
        # Status: 0x80 (Active)
        mbr[446] = 0x80

        # CHS Start (Head 0, Sector 2, Cylinder 0 ? No. LBA 2048)
        # Geometry assumption: H=64, S=32.
        # LBA 2048 -> C=1, H=0, S=1.
        # Cylinder 1 (0x01). Head 0 (0x00). Sector 1 (0x01).
        mbr[447] = 0x00 # Head
        mbr[448] = 0x01 # Sector (bits 0-5) | CylHigh (bits 6-7)
        mbr[449] = 0x01 # CylLow

        # Type: 0xEF (EFI)
        mbr[450] = 0xEF

        # CHS End (Calculate)
        # End LBA = Start + Size - 1
        end_lba = start_lba + fat_sectors - 1
        # C = End / (64*32) = End / 2048
        # H = (End / 32) % 64
        # S = (End % 32) + 1

        c = end_lba // 2048
        h = (end_lba // 32) % 64
        s = (end_lba % 32) + 1

        if c > 1023:
            # Max CHS
            mbr[451] = 0xFE # Head
            mbr[452] = 0xFF # Sector/CylHigh
            mbr[453] = 0xFF # CylLow
        else:
            mbr[451] = h
            # Sector byte: s | ((c >> 8) & 0x03) << 6
            mbr[452] = (s & 0x3F) | (((c >> 8) & 0x03) << 6)
            mbr[453] = c & 0xFF

        # LBA Start (4 bytes, little endian)
        struct.pack_into('<I', mbr, 454, start_lba)

        # LBA Size (4 bytes, little endian)
        struct.pack_into('<I', mbr, 458, fat_sectors)

        # Signature
        mbr[510] = 0x55
        mbr[511] = 0xAA

        f.write(mbr)

        # Write Padding (from Sector 1 to 2047)
        # 2048 - 1 = 2047 sectors padding
        padding_size = (start_lba - 1) * sector_size
        # We can just seek
        f.seek(start_lba * sector_size)

        # Append FAT Volume
        with open(temp_fat, 'rb') as tf:
            shutil.copyfileobj(tf, f)

    # Cleanup
    os.remove(temp_fat)
    print(f"Created partitioned disk image at {image_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: make_fat.py <image_path> <efi_file_path>")
        sys.exit(1)

    create_partitioned_image(sys.argv[1], sys.argv[2])
