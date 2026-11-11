"""Render 320x240 previews of the four screens, pixel-accurate to the firmware.

These are drawings, not photographs: same coordinates, colours (RGB565 converted)
and font sizes as the code, but the glyphs come from a desktop font. They exist so
the repository has images without pretending to be camera shots.

Output: docs/screens/*.png
"""

import os
from PIL import Image, ImageDraw, ImageFont

W, H = 320, 240
OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "screens")

FONT_BOLD = r"C:\Windows\Fonts\arialbd.ttf"
FONT_REG = r"C:\Windows\Fonts\arial.ttf"


def rgb565(value):
    r = (value >> 11) & 0x1F
    g = (value >> 5) & 0x3F
    b = value & 0x1F
    return (r * 255 // 31, g * 255 // 63, b * 255 // 31)


CYAN = rgb565(0x07FF)
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
DARK_SLATE = rgb565(0x18E3)
GREY = rgb565(0x7BEF)
OUTLINE = rgb565(0x4208)


def font(size, bold=True):
    return ImageFont.truetype(FONT_BOLD if bold else FONT_REG, size)


def new_canvas():
    img = Image.new("RGB", (W, H), BLACK)
    return img, ImageDraw.Draw(img)


def eyes_preview():
    img, d = new_canvas()
    # NORMAL preset: rx 48, ry 56, pupil 16, dy 0, spacing 68, brow lift 12, len 46
    for cx in (160 - 68, 160 + 68):
        cy = 120
        rx, ry = 48, 56
        corner = int(min(rx, ry) * 0.55)
        d.rounded_rectangle([cx - rx, cy - ry, cx + rx, cy + ry], radius=corner,
                            fill=CYAN)
        d.ellipse([cx - 16, cy - 16, cx + 16, cy + 16], fill=BLACK)
        d.ellipse([cx - 7 - 6, cy - 7 - 6, cx - 7 + 6, cy - 7 + 6], fill=WHITE)

        # Brow: straight line with a parabolic arch, drawn as 3 px dots.
        top = cy - ry - 12
        length = 46
        x0 = cx - length // 2
        for i in range(length + 1):
            t = i / length
            y = top - 4 * 4.0 * t * (1 - t)
            d.ellipse([x0 + i - 3, y - 3, x0 + i + 3, y + 3], fill=CYAN)
    return img


def button(d, x, y, w, h, label, selected, size=12):
    if selected:
        d.rectangle([x, y, x + w, y + h], fill=CYAN)
        colour = BLACK
    else:
        d.rectangle([x, y, x + w, y + h], outline=OUTLINE)
        colour = GREY
    d.text((x + w // 2, y + h // 2), label, font=font(size), fill=colour, anchor="mm")


def settings_preview():
    img, d = new_canvas()
    d.text((8, 8), "SETTINGS", font=font(12), fill=CYAN)

    rows = [(44, "Brightness", ["100%", "40%", "20%"], 0),
            (92, "Screen off", ["1m", "3m", "10m"], 1),
            (140, "Eye colour", ["cyan", "green", "amber", "white"], 2)]
    for y, title, labels, sel in rows:
        d.text((8, y + 18), title, font=font(12), fill=GREY, anchor="lm")
        for i, label in enumerate(labels):
            button(d, 100 + i * 54, y, 50, 36, label, i == sel, 11)

    d.text((160, 212), "swipe right to close", font=font(11), fill=GREY, anchor="mm")
    return img


def pomodoro_preview():
    img, d = new_canvas()
    d.text((8, 8), "POMODORO", font=font(12), fill=CYAN)
    d.text((312, 8), "DONE 2", font=font(12), fill=CYAN, anchor="ra")
    d.text((160, 108), "24:13", font=font(46), fill=CYAN, anchor="mm")
    d.text((160, 160), "WORK", font=font(20), fill=CYAN, anchor="mm")
    d.text((160, 214), "tap: start/stop   down: reset   up: exit",
           font=font(10), fill=GREY, anchor="mm")
    return img


TILE_COLOURS = [0x05FF, 0x04DF, 0x03BF, 0x029F, 0x019F, 0x00DF, 0x0D3F,
                0x0C1F, 0x0AFF, 0x09DF, 0x7FE0, 0x7C00, 0xF800, 0xF000, 0xE000]

BOARD = [
    [2, 8, 32, 4],
    [16, 64, 128, 2],
    [4, 256, 512, 8],
    [2, 16, 32, 64],
]


def tile_colour(value):
    if value == 0:
        return DARK_SLATE
    exp = 0
    while (1 << exp) < value:
        exp += 1
    return rgb565(TILE_COLOURS[min(exp - 1, len(TILE_COLOURS) - 1)])


def game_preview():
    img, d = new_canvas()
    # Board background: origin (56,20), 4 cells of 48 with 6 px gaps.
    d.rectangle([56, 20, 56 + 222, 20 + 222], fill=DARK_SLATE)
    for r in range(4):
        for c in range(4):
            value = BOARD[r][c]
            x, y = 62 + c * 54, 26 + r * 54
            d.rectangle([x, y, x + 48, y + 48], fill=tile_colour(value))
            if value:
                colour = BLACK if value <= 4 else WHITE
                size = 18 if value < 1000 else 13
                d.text((x + 24, y + 24), str(value), font=font(size), fill=colour,
                       anchor="mm")

    # Exit button
    d.rectangle([2, 2, 62, 48], fill=DARK_SLATE)
    d.text((32, 25), "<", font=font(20), fill=CYAN, anchor="mm")

    d.text((316, 8), "1284  best 3296", font=font(11), fill=CYAN, anchor="ra")
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, fn in (("eyes.png", eyes_preview),
                     ("settings.png", settings_preview),
                     ("pomodoro.png", pomodoro_preview),
                     ("game2048.png", game_preview)):
        path = os.path.join(OUT, name)
        fn().save(path)
        print("wrote", path)


if __name__ == "__main__":
    main()
