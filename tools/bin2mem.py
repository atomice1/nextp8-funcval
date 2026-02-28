#!/usr/bin/env python3
"""
Convert binary file to Verilog .mem format
Usage: bin2mem.py input.bin output.mem
"""

import sys
import os

def bin2mem(input_file, output_file):
    """Convert binary file to .mem format (hex 16-bit words, one per line)"""
    try:
        with open(input_file, 'rb') as f:
            data = f.read()

        if len(data) % 2 != 0:
            data += b'\x00'

        with open(output_file, 'w') as f:
            for i in range(0, len(data), 2):
                word = (data[i] << 8) | data[i + 1]
                f.write(f'{word:04x}\n')

        print(f'Converted {input_file} to {output_file}')
        print(f'  Size: {len(data)} bytes')

    except FileNotFoundError:
        print(f'Error: File not found: {input_file}', file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f'Error: {e}', file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} input.bin output.mem', file=sys.stderr)
        sys.exit(1)

    bin2mem(sys.argv[1], sys.argv[2])
