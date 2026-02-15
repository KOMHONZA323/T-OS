import struct
import sys
import os

def pad(data, size):
    if len(data) > size:
        return data[:size]
    return data + b'\x00' * (size - len(data))

def make_fat16_image(boot_bin, kernel_bin, output_img, files):
    # Standard 1.44MB Floppy Geometry
    SECTOR_SIZE = 512
    SECTORS_PER_CLUSTER = 1
    TOTAL_SECTORS = 2880 # 1.44MB

    with open(kernel_bin, 'rb') as f:
        kernel_data = f.read()

    kernel_sectors = (len(kernel_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    RESERVED_SECTORS = 1 + kernel_sectors

    FAT_COPIES = 2
    ROOT_DIR_ENTRIES = 224
    MEDIA_DESCRIPTOR = 0xF0
    SECTORS_PER_FAT = 9

    # Calculate offsets
    fat1_start = RESERVED_SECTORS * SECTOR_SIZE
    fat2_start = fat1_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_start = fat2_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_size = ROOT_DIR_ENTRIES * 32
    data_start = root_start + root_size

    # Create blank image
    TARGET_SIZE = TOTAL_SECTORS * SECTOR_SIZE
    image = bytearray(TARGET_SIZE)

    # --- 1. Boot Sector ---
    with open(boot_bin, 'rb') as f:
        boot_code = f.read()
    boot_code = pad(boot_code, 512)

    # Patch BPB
    # OEM Name (3-10)
    image[0:512] = boot_code # Copy first
    image[3:11] = b'T-OS    '
    image[11:13] = struct.pack('<H', SECTOR_SIZE)
    image[13] = SECTORS_PER_CLUSTER
    image[14:16] = struct.pack('<H', RESERVED_SECTORS)
    image[16] = FAT_COPIES
    image[17:19] = struct.pack('<H', ROOT_DIR_ENTRIES)
    image[19:21] = struct.pack('<H', TOTAL_SECTORS)
    image[21] = MEDIA_DESCRIPTOR
    image[22:24] = struct.pack('<H', SECTORS_PER_FAT)
    image[24:26] = struct.pack('<H', 18) # SPT
    image[26:28] = struct.pack('<H', 2)  # Heads
    image[28:32] = struct.pack('<I', 0)  # Hidden
    image[32:36] = struct.pack('<I', 0)  # Large Sectors
    image[36] = 0x80 # Drive 0
    image[37] = 0
    image[38] = 0x29
    image[39:43] = struct.pack('<I', 0x12345678)
    image[43:54] = b'T-OS DISK  '
    image[54:62] = b'FAT12   '

    # --- 2. Reserved Sectors (Kernel) ---
    # Check if kernel fits before FAT starts
    if 512 + len(kernel_data) > fat1_start:
        print(f"Error: Kernel too large! {len(kernel_data)} bytes. Reserved: {RESERVED_SECTORS} sectors.")
        sys.exit(1)

    image[512 : 512 + len(kernel_data)] = kernel_data

    # --- 3. FAT Tables ---
    image[fat1_start : fat1_start+4] = b'\xF0\xFF\xFF\xFF'
    image[fat2_start : fat2_start+4] = b'\xF0\xFF\xFF\xFF'

    # --- 4. Files ---
    current_cluster = 2
    dir_offset = root_start

    for filename, path in files.items():
        if not os.path.exists(path):
            print(f"Warning: File {path} not found.")
            continue

        with open(path, 'rb') as f:
            data = f.read()

        size = len(data)

        # Dir Entry
        name, ext = filename.split('.')
        entry = bytearray(32)
        entry[0:8] = pad(name.encode().upper(), 8)
        entry[8:11] = pad(ext.encode().upper(), 3)
        entry[11] = 0x20
        entry[26:28] = struct.pack('<H', current_cluster)
        entry[28:32] = struct.pack('<I', size)

        image[dir_offset : dir_offset+32] = entry
        dir_offset += 32

        # Write Data
        sectors_needed = (size + SECTOR_SIZE - 1) // SECTOR_SIZE
        for i in range(sectors_needed):
            # Write Sector
            cluster_offset = data_start + ((current_cluster - 2) * SECTOR_SIZE * SECTORS_PER_CLUSTER)
            chunk = data[i*SECTOR_SIZE : (i+1)*SECTOR_SIZE]

            if cluster_offset + len(chunk) > TARGET_SIZE:
                print("Error: Disk full!")
                sys.exit(1)

            image[cluster_offset : cluster_offset+len(chunk)] = chunk

            # Update FAT
            fat_off = fat1_start + (current_cluster * 2) # FAT16 entries are 2 bytes?
            # Wait, I said 'FAT12   ' string but I am using 2-byte entries (FAT16)?
            # FAT12 uses 12-bits (1.5 bytes). FAT16 uses 16-bits (2 bytes).
            # If I write 2 bytes, I MUST set type to FAT16 or OS will read garbage.
            # But floppy is usually FAT12.
            #  in kernel likely implements FAT16.
            # So I should use 'FAT16   ' in BPB.
            # And  uses 2-byte entries. Correct.

            # Next cluster
            next_cl = 0xFFFF if i == sectors_needed - 1 else current_cluster + 1
            image[fat_off : fat_off+2] = struct.pack('<H', next_cl)

            # FAT2
            fat2_off = fat2_start + (current_cluster * 2)
            image[fat2_off : fat2_off+2] = struct.pack('<H', next_cl)

            current_cluster += 1

    # Fix BPB String to FAT16 to be consistent
    image[54:62] = b'FAT16   '

    # Assert Size
    if len(image) != TARGET_SIZE:
        print(f"Error: Image size mismatch! Expected {TARGET_SIZE}, got {len(image)}")
        sys.exit(1)

    with open(output_img, 'wb') as f:
        f.write(image)

    print(f"Success: Created {output_img} ({len(image)} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: make_fat.py <boot> <kernel> <out> [files...]")
        sys.exit(1)

    files = {}
    for arg in sys.argv[4:]:
        if '=' in arg:
            k,v = arg.split('=')
            files[k] = v

    make_fat16_image(sys.argv[1], sys.argv[2], sys.argv[3], files)
