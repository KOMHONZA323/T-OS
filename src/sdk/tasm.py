#!/usr/bin/env python3
import sys
import struct
import re

class TASM:
    def __init__(self):
        self.labels = {}
        self.lines = []
        self.code = bytearray()
        self.pc = 0

    def parse_file(self, filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.split(';')[0].strip()  # Remove comments
                if not line: continue

                # Handle Labels
                if line.endswith(':'):
                    label = line[:-1]
                    self.labels[label] = self.pc
                    continue

                # Handle Directives
                parts = re.split(r'[ ,]+', line)
                if parts[0] in ['global', 'extern', 'section', 'bits']:
                    continue

                self.lines.append((self.pc, line))
                self.pc += self.get_instruction_size(line)

    def get_instruction_size(self, line):
        # Precise size calculation for Pass 1
        parts = re.split(r'[ ,]+', line)
        opcode = parts[0].lower()

        if opcode == 'mov':
            dest = parts[1]
            src = parts[2]
            # MOV reg, imm32 (B8+rd imm32) -> 5 bytes
            if src.startswith('0x') or src.isdigit(): return 5
            # MOV reg, [esp+offset] (8B 44 24 disp8) -> 4 bytes (simplified for small offsets < 128)
            # Actually, standard ModRM/SIB for [esp+disp8] is 4 bytes: Op(8B) ModRM(44) SIB(24) Disp8
            if src.startswith('[esp+') and src.endswith(']'): return 4
            # MOV reg, reg (89 modrm) -> 2 bytes
            return 2

        elif opcode == 'push':
            src = parts[1]
            if src.startswith('0x') or src.isdigit():
                val = int(src, 0)
                if val < 128: return 2 # 6A imm8
                return 5 # 68 imm32
            return 1 # 50+rd

        elif opcode == 'pop':
            return 1 # 58+rd

        elif opcode == 'call':
            return 5 # E8 rel32

        elif opcode == 'int':
            return 2 # CD imm8

        elif opcode == 'ret':
            return 1 # C3

        elif opcode == 'add':
            if parts[1] == 'esp':
                val = int(parts[2], 0)
                if val < 128: return 3 # 83 C4 imm8
                return 6 # 81 C4 imm32
            return 0 # Unknown add

        elif opcode == 'jmp':
            return 5 # E9 rel32

        return 0

    def assemble(self):
        # Pass 2: Generate Code
        self.code = bytearray()
        current_pc = 0

        for _, line in self.lines:
            bytes_code = self.assemble_line(line, current_pc)
            self.code.extend(bytes_code)
            current_pc += len(bytes_code)

        # Generate Header
        header = bytearray()
        header.extend(b'TOS') # Magic

        # Entry Point
        entry = self.labels.get('_start', 0)
        header.extend(struct.pack('<I', entry))

        # Code Size
        header.extend(struct.pack('<I', len(self.code)))

        # Data Size (0)
        header.extend(struct.pack('<I', 0))

        return header + self.code

    def assemble_line(self, line, pc):
        parts = re.split(r'[ ,]+', line)
        opcode = parts[0].lower()
        regs = {'eax': 0, 'ecx': 1, 'edx': 2, 'ebx': 3, 'esp': 4, 'ebp': 5, 'esi': 6, 'edi': 7}

        if opcode == 'mov':
            dest = parts[1]
            src = parts[2]

            if dest in regs and (src.startswith('0x') or src.isdigit()):
                val = int(src, 0)
                return bytes([0xB8 + regs[dest]]) + struct.pack('<I', val)

            if dest in regs and src.startswith('[esp+') and src.endswith(']'):
                offset = int(src[5:-1], 0)
                modrm = 0x44 | (regs[dest] << 3)
                return bytes([0x8B, modrm, 0x24, offset])

            if dest in regs and src in regs:
                modrm = 0xC0 | (regs[src] << 3) | regs[dest]
                return bytes([0x89, modrm])

        elif opcode == 'push':
            src = parts[1]
            if src in regs:
                return bytes([0x50 + regs[src]])
            elif src.startswith('0x') or src.isdigit():
                val = int(src, 0)
                if val < 128:
                    return bytes([0x6A, val])
                else:
                    return bytes([0x68]) + struct.pack('<I', val)

        elif opcode == 'pop':
            dest = parts[1]
            if dest in regs:
                return bytes([0x58 + regs[dest]])

        elif opcode == 'call':
            target = parts[1]
            target_addr = self.labels.get(target, 0) # Default to 0 if extern
            # E8 rel32
            rel = target_addr - (pc + 5)
            return bytes([0xE8]) + struct.pack('<i', rel)

        elif opcode == 'int':
            val = int(parts[1], 0)
            return bytes([0xCD, val])

        elif opcode == 'ret':
            return bytes([0xC3])

        elif opcode == 'add':
            if parts[1] == 'esp':
                val = int(parts[2], 0)
                if val < 128:
                    return bytes([0x83, 0xC4, val])
                else:
                    return bytes([0x81, 0xC4]) + struct.pack('<I', val)

        elif opcode == 'jmp':
            target = parts[1]
            target_addr = self.labels.get(target, 0)
            rel = target_addr - (pc + 5)
            return bytes([0xE9]) + struct.pack('<i', rel)

        return b''

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: tasm.py <input.asm>... <output.texf>")
        sys.exit(1)

    tasm = TASM()
    inputs = sys.argv[1:-1]
    output = sys.argv[-1]

    for f in inputs:
        tasm.parse_file(f)

    binary = tasm.assemble()

    with open(output, 'wb') as f:
        f.write(binary)
