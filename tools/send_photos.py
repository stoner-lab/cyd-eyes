"""Send every JPEG in a folder to the board's microSD over the serial line.

The laptop has no card reader and the microSD sits inside the CYD, so the
firmware exposes a tiny file receiver ("PUT <name> <bytes>" + raw data).

Phone photos are cropped to 4:3 and resized to 320x240 first: the panel is
320x240 and the JPEG decoder can only halve dimensions.

Usage: python send_photos.py [COM3]
"""

import io
import os
import sys
import time

import serial
from PIL import Image, ImageOps

SOURCE = r"C:\Users\duy\Desktop\cyd"
TARGET_W, TARGET_H = 320, 240
CHUNK = 512
CHUNK_PAUSE = 0.06  # 115200 baud is ~11.5 kB/s; this stays under it


def prepare(path):
    image = Image.open(path)
    image = ImageOps.exif_transpose(image)

    # Shrink in integer steps first: LANCZOS straight from 4000 px to 320 px
    # aliases detail into mush, which reads as a blurry photo on the panel.
    factor = max(1, min(image.width // TARGET_W, image.height // TARGET_H))
    if factor > 1:
        image = image.reduce(factor)

    image = ImageOps.fit(image, (TARGET_W, TARGET_H), Image.LANCZOS)

    buffer = io.BytesIO()
    # subsampling=0 keeps colour at full resolution; 4:2:0 smears small details.
    image.convert("RGB").save(
        buffer, "JPEG", quality=92, subsampling=0, optimize=True
    )
    return buffer.getvalue()


def send(port, name, payload):
    port.write(f"PUT {name} {len(payload)}\n".encode("ascii"))
    time.sleep(0.15)

    for offset in range(0, len(payload), CHUNK):
        port.write(payload[offset:offset + CHUNK])
        port.flush()
        time.sleep(CHUNK_PAUSE)

    deadline = time.time() + 15
    while time.time() < deadline:
        line = port.readline().decode("ascii", "ignore").strip()
        if line.startswith("OK ") or line.startswith("ERR "):
            return line
    return "timeout"


def main():
    port_name = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    files = sorted(f for f in os.listdir(SOURCE) if f.lower().endswith(".jpg"))
    if not files:
        raise SystemExit(f"no jpg in {SOURCE}")

    port = serial.Serial(port_name, 115200, timeout=1)
    port.dtr = False
    port.rts = False
    time.sleep(0.4)

    sent = 0
    for name in files:
        payload = prepare(os.path.join(SOURCE, name))
        short = os.path.splitext(name)[0][:40] + ".jpg"
        reply = send(port, short, payload)
        print(f"{name} -> {reply} ({len(payload)} bytes)", flush=True)
        if reply.startswith("OK "):
            sent += 1

    port.close()
    print(f"done: {sent}/{len(files)} files")


if __name__ == "__main__":
    main()
