import sys
import struct
import os

def create_uefi_image(efi_file, output_image):
    # FAT12 Floppy 1.44MB Geometry
    SECTOR_SIZE = 512
    SECTORS_PER_CLUSTER = 1
    RESERVED_SECTORS = 1
    FAT_COPIES = 2
    ROOT_ENTRIES = 224
    TOTAL_SECTORS = 2880
    SECTORS_PER_FAT = 9
    MEDIA_DESCRIPTOR = 0xF0

    image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)

    # 1. BPB
    # Jump + NOP
    image[0:3] = b'\xEB\x3C\x90'
    image[3:11] = b'MSDOS5.0' # OEM
    image[11:13] = struct.pack('<H', SECTOR_SIZE)
    image[13] = SECTORS_PER_CLUSTER
    image[14:16] = struct.pack('<H', RESERVED_SECTORS)
    image[16] = FAT_COPIES
    image[17:19] = struct.pack('<H', ROOT_ENTRIES)
    image[19:21] = struct.pack('<H', TOTAL_SECTORS)
    image[21] = MEDIA_DESCRIPTOR
    image[22:24] = struct.pack('<H', SECTORS_PER_FAT)
    image[24:26] = struct.pack('<H', 18) # SPT
    image[26:28] = struct.pack('<H', 2)  # Heads
    image[28:32] = struct.pack('<I', 0)  # Hidden
    image[36] = 0x00 # Drive 0
    image[38] = 0x29 # Signature
    image[39:43] = struct.pack('<I', 0x12345678) # Serial
    image[43:54] = b'NO NAME    '
    image[54:62] = b'FAT12   '
    image[510:512] = b'\x55\xAA'

    # Layout calculations
    fat1_start = RESERVED_SECTORS * SECTOR_SIZE
    fat2_start = fat1_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_start = fat2_start + (SECTORS_PER_FAT * SECTOR_SIZE)
    root_size_bytes = ROOT_ENTRIES * 32
    data_start = root_start + root_size_bytes

    def write_fat(cluster, value):
        # 12-bit FAT entry packing
        offset = int(cluster * 1.5)

        # Access both FAT copies
        for start in [fat1_start, fat2_start]:
            pos = start + offset

            if cluster % 2 == 0:
                # Even: low 8 bits in b1, high 4 bits in low nibble of b2
                image[pos] = value & 0xFF
                image[pos+1] = (image[pos+1] & 0xF0) | ((value >> 8) & 0x0F)
            else:
                # Odd: low 4 bits in high nibble of b1, high 8 bits in b2
                image[pos] = (image[pos] & 0x0F) | ((value & 0x0F) << 4)
                image[pos+1] = (value >> 4) & 0xFF

    # Initialize FAT ID
    write_fat(0, 0xF00 | MEDIA_DESCRIPTOR)
    write_fat(1, 0xFFF)

    next_free_cluster = 2

    def alloc_cluster():
        nonlocal next_free_cluster
        c = next_free_cluster
        next_free_cluster += 1
        return c

    def write_dir_entry(offset, name, ext, attr, cluster, size):
        entry = bytearray(32)
        entry[0:8] = (name + " "*8)[:8].encode()
        entry[8:11] = (ext + " "*3)[:3].encode()
        entry[11] = attr
        entry[26:28] = struct.pack('<H', cluster)
        entry[28:32] = struct.pack('<I', size)
        image[offset : offset+32] = entry

    # Create Directories
    # Root -> EFI (Cluster 2)
    efi_cluster = alloc_cluster()
    write_fat(efi_cluster, 0xFFF) # End of chain
    write_dir_entry(root_start, "EFI", "", 0x10, efi_cluster, 0)

    # EFI -> BOOT (Cluster 3)
    boot_cluster = alloc_cluster()
    write_fat(boot_cluster, 0xFFF)

    # Calculate offset of EFI dir data
    # Cluster N data is at data_start + (N-2)*SECTOR_SIZE
    efi_dir_offset = data_start + (efi_cluster - 2) * SECTOR_SIZE

    # . and .. entries in EFI
    write_dir_entry(efi_dir_offset, ".", "", 0x10, efi_cluster, 0)
    write_dir_entry(efi_dir_offset+32, "..", "", 0x10, 0, 0) # Parent is Root (0)
    write_dir_entry(efi_dir_offset+64, "BOOT", "", 0x10, boot_cluster, 0)

    # BOOT -> BOOTX64.EFI (Cluster 4+)
    boot_dir_offset = data_start + (boot_cluster - 2) * SECTOR_SIZE
    write_dir_entry(boot_dir_offset, ".", "", 0x10, boot_cluster, 0)
    write_dir_entry(boot_dir_offset+32, "..", "", 0x10, efi_cluster, 0)

    # Read File
    with open(efi_file, 'rb') as f:
        file_data = f.read()
    file_size = len(file_data)

    # Write File Data
    start_cluster = alloc_cluster()
    current_cluster = start_cluster
    sectors_needed = (file_size + SECTOR_SIZE - 1) // SECTOR_SIZE

    # Write BOOTX64.EFI entry
    write_dir_entry(boot_dir_offset+64, "BOOTX64", "EFI", 0x20, start_cluster, file_size)

    for i in range(sectors_needed):
        # Write sector
        data_offset = data_start + (current_cluster - 2) * SECTOR_SIZE
        chunk = file_data[i*SECTOR_SIZE : (i+1)*SECTOR_SIZE]
        image[data_offset : data_offset + len(chunk)] = chunk

        # FAT Chain
        if i < sectors_needed - 1:
            next_c = alloc_cluster()
            write_fat(current_cluster, next_c)
            current_cluster = next_c
        else:
            write_fat(current_cluster, 0xFFF) # End of file

    with open(output_image, 'wb') as f:
        f.write(image)

    print(f"Created {output_image} with EFI/BOOT/BOOTX64.EFI")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: make_uefi_image.py <input.efi> <output.img>")
        sys.exit(1)
    create_uefi_image(sys.argv[1], sys.argv[2])
