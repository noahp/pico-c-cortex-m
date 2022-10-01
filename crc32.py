#!/usr/bin/env python3

"""
Print the CRC32 checksum of a file.
"""

import sys, zlib

with open(sys.argv[1], "rb") as f:
    print(hex(zlib.crc32(f.read())))
