import struct
import sys
import os

def pad(data, size):
    return data + b'\x00' * (size - len(data))

def make_fat16_image(boot_bin, kernel_bin, output_img, files):
    # Standard 1.44MB Floppy Geometry
    SECTOR_SIZE = 512
    SECTORS_PER_CLUSTER = 1
    RESERVED_SECTORS = 1 # Boot sector only for now? No, we need space for kernel.
    # We will put Stage 2 + Kernel in Reserved Sectors.
    # Kernel size? Let's say 256 sectors (128KB).
    # So Reserved Sectors = 1 (Boot) + X (Kernel).

    with open(kernel_bin, 'rb') as f:
        kernel_data = f.read()

    kernel_sectors = (len(kernel_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    RESERVED_SECTORS = 1 + kernel_sectors

    FAT_COPIES = 2
    ROOT_DIR_ENTRIES = 224
    TOTAL_SECTORS = 2880 # 1.44MB
    MEDIA_DESCRIPTOR = 0xF0
    SECTORS_PER_FAT = 9

    # Calculate offsets
    fat1_start = RESERVED_SECTORS * SECTOR_SIZE
    fat2_start = fat1_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_start = fat2_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_size = ROOT_DIR_ENTRIES * 32
    data_start = root_start + root_size

    # Create blank image
    image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)

    # --- 1. Boot Sector (Patched with BPB) ---
    with open(boot_bin, 'rb') as f:
        boot_code = bytearray(f.read())
        if len(boot_code) > 512: boot_code = boot_code[:512]
        boot_code = pad(boot_code, 512)

    # Patch BPB (Offset 3 or 11 depending on structure, assume standard layout)
    # OEM Name (3-10)
    boot_code[3:11] = b'T-OS    '
    # Bytes Per Sector (11-12)
    boot_code[11:13] = struct.pack('<H', SECTOR_SIZE)
    # Sectors Per Cluster (13)
    boot_code[13] = SECTORS_PER_CLUSTER
    # Reserved Sectors (14-15)
    boot_code[14:16] = struct.pack('<H', RESERVED_SECTORS)
    # FAT Copies (16)
    boot_code[16] = FAT_COPIES
    # Root Dir Entries (17-18)
    boot_code[17:19] = struct.pack('<H', ROOT_DIR_ENTRIES)
    # Total Sectors 16 (19-20)
    boot_code[19:21] = struct.pack('<H', TOTAL_SECTORS)
    # Media Descriptor (21)
    boot_code[21] = MEDIA_DESCRIPTOR
    # Sectors Per FAT (22-23)
    boot_code[22:24] = struct.pack('<H', SECTORS_PER_FAT)
    # Sectors Per Track (24-25) -> 18
    boot_code[24:26] = struct.pack('<H', 18)
    # Heads (26-27) -> 2
    boot_code[26:28] = struct.pack('<H', 2)
    # Hidden Sectors (28-31) -> 0
    boot_code[28:32] = struct.pack('<I', 0)
    # Total Sectors 32 (32-35) -> 0
    boot_code[32:36] = struct.pack('<I', 0)

    # Extended BPB (Offset 36 for FAT12/16)
    # Drive Number (36) -> 0x00 (Floppy) or 0x80 (HDD)
    # Since boot_sect.asm loads from DL, we use 0x00 usually for floppy.
    boot_code[36] = 0x00
    # Reserved (37) -> 0
    boot_code[37] = 0
    # Boot Signature (38) -> 0x29
    boot_code[38] = 0x29
    # Volume ID (39-42)
    boot_code[39:43] = struct.pack('<I', 0x12345678)
    # Volume Label (43-53)
    boot_code[43:54] = b'T-OS DISK  '
    # FS Type (54-61)
    boot_code[54:62] = b'FAT12   ' # Usually FAT12 for floppy, but we use FAT16 structures in code?
    # Kernel code uses "FAT16" strings? Let's assume FAT12 compatible BPB.

    image[0:512] = boot_code

    # --- 2. Write Kernel to Reserved Sectors ---
    # Start at sector 1 (Offset 512)
    image[512 : 512 + len(kernel_data)] = kernel_data

    # --- 3. Initialize FAT ---
    # First 2 entries reserved: F0 FF FF (FAT12) or F0 FF FF FF (FAT16)
    # Let's assume FAT12 for 1.44MB floppy (standard).
    # Entry 0: Media Descriptor (F0) + FF + FF (Cluster 0/1 reserved)
    # FAT12 logic is messy (1.5 bytes per entry).
    # FAT16 logic is easier (2 bytes per entry).
    # If we force FAT16 on floppy:
    # Set FS Type to 'FAT16   '.
    # FAT entries are 2 bytes.
    # Entry 0: F0 FF. Entry 1: FF FF.
    image[fat1_start : fat1_start+4] = b'\xF0\xFF\xFF\xFF'
    image[fat2_start : fat2_start+4] = b'\xF0\xFF\xFF\xFF'

    # --- 4. Write Files ---
    current_cluster = 2 # Start data clusters at 2
    dir_offset = root_start

    for filename, path in files.items():
        with open(path, 'rb') as f:
            data = f.read()

        size = len(data)
        start_cluster = current_cluster

        # Write Directory Entry (32 bytes)
        # Name (8) + Ext (3)
        name, ext = filename.split('.')
        entry = bytearray(32)
        entry[0:8] = pad(name.encode().upper(), 8)
        entry[8:11] = pad(ext.encode().upper(), 3)
        # Attr (11) -> Archive (0x20)
        entry[11] = 0x20
        # Time/Date (22-25) -> Dummy
        entry[22:24] = b'\x00\x00'
        entry[24:26] = b'\x00\x00'
        # First Cluster (26-27)
        entry[26:28] = struct.pack('<H', start_cluster)
        # Size (28-31)
        entry[28:32] = struct.pack('<I', size)

        image[dir_offset : dir_offset+32] = entry
        dir_offset += 32

        # Write Data to Clusters
        # Simplified: Contiguous allocation
        # For each sector of data, occupy a cluster (since 1 sector/cluster)
        sectors_needed = (size + SECTOR_SIZE - 1) // SECTOR_SIZE

        # Update FAT Chain
        for i in range(sectors_needed):
            cluster_offset = data_start + ((current_cluster - 2) * SECTOR_SIZE * SECTORS_PER_CLUSTER)
            chunk = data[i*SECTOR_SIZE : (i+1)*SECTOR_SIZE]
            image[cluster_offset : cluster_offset+len(chunk)] = chunk

            # Set FAT entry for current_cluster
            # Point to next, or End (FFFF)
            fat_offset = fat1_start + (current_cluster * 2) # FAT16
            if i == sectors_needed - 1:
                next_val = 0xFFFF
            else:
                next_val = current_cluster + 1

            image[fat_offset : fat_offset+2] = struct.pack('<H', next_val)

            # Copy to FAT2
            fat2_offset = fat2_start + (current_cluster * 2)
            image[fat2_offset : fat2_offset+2] = struct.pack('<H', next_val)

            current_cluster += 1

    # Write Image
    with open(output_img, 'wb') as f:
        f.write(image)

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: make_fat.py <boot.bin> <kernel.bin> <output.img> <file1=path1> ...")
        sys.exit(1)

    boot_bin = sys.argv[1]
    kernel_bin = sys.argv[2]
    output_img = sys.argv[3]
    files = {}
    for arg in sys.argv[4:]:
        name, path = arg.split('=')
        files[name] = path

    make_fat16_image(boot_bin, kernel_bin, output_img, files)
