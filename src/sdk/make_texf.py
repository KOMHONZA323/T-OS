import struct
import sys

def make_texf(input_file, output_file):
    with open(input_file, 'rb') as f:
        data = f.read()

    # Header: Magic(3)+Ver(1)+CodeOff(4)+CodeSize(4)+DataOff(4)+DataSize(4)+Entry(4)
    # Magic: T O S (0x54 0x4F 0x53)
    # Version: 1
    # Code Offset: sizeof(header) = 3+1+4+4+4+4+4 = 24 bytes
    # Code Size: len(data)
    # Data Offset: 0 (No data segment separation yet)
    # Data Size: 0
    # Entry Point: 0 (Relative to start of code segment, so execution starts at first instruction)

    header = struct.pack('<3sBIIIII', b'TOS', 1, 24, len(data), 0, 0, 0)

    with open(output_file, 'wb') as f:
        f.write(header + data)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: make_texf.py <input.bin> <output.texf>")
        sys.exit(1)

    make_texf(sys.argv[1], sys.argv[2])
