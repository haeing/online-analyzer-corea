"""Write one full-readout HDDAQ event with identifiable peaks on every channel."""
import struct
import sys
from pathlib import Path

blocks = []
for asad in range(4):
    payload = bytearray()
    for bucket in range(512):
        for channel in range(68):
            for aget in range(4):
                adc = (800 + asad * 100 + aget * 10 + channel
                       if bucket == 300 + channel % 8 else 100)
                payload += struct.pack('>H', (aget << 14) | adc)
    header = bytearray(256)
    header[1:4] = ((256 + len(payload)) // 256).to_bytes(3, 'big')
    header[5:7] = (2).to_bytes(2, 'big')
    header[7] = 5
    header[8:10] = (1).to_bytes(2, 'big')
    header[10:12] = (2).to_bytes(2, 'big')
    header[12:16] = (len(payload) // 2).to_bytes(4, 'big')
    header[22:26] = (1).to_bytes(4, 'big')
    header[27] = asad
    blocks.append(struct.pack('<II', 0x41734164, (264 + len(payload)) // 4)
                  + header + payload)
body = b''.join(blocks)
cobo = struct.pack('<4I', 0x436F426F, 0, len(body) // 4 + 4, 4) + body
node = struct.pack('<8I', 0x45564E54, 8 + len(cobo) // 4, 1, 24, 0xCB0, 0, 1, 0) + cobo
event = struct.pack('<8I', 0x45564E54, 8 + len(node) // 4, 1, 24, 1, 0, 1, 0) + node
Path(sys.argv[1]).write_bytes(event)
