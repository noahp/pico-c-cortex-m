#!/usr/bin/env python3

import sys
import struct
import json
import zlib

# read calibration data from json file and pack into a binary file
if len(sys.argv) < 3:
    print("error, pass both json file and output file as args", file=sys.stderr)
    exit(1)

with open(sys.argv[1]) as jsonfile:
    jsondata = json.load(jsonfile)

binarydata = struct.pack("<II", jsondata["upper_cal"], jsondata["lower_cal"])

# add a crc32
binarydata = binarydata + struct.pack("<I", zlib.crc32(binarydata))

with open(sys.argv[2], "wb") as binfile:
    binfile.write(binarydata)
