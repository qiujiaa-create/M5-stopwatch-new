#!/usr/bin/env python3
"""
把一张正方形 PNG 转成 LVGL 的 C 数组（LV_COLOR_FORMAT_RGB565），
用于 StopWatch 固件的启动器图标（main/assets/images/icon_*.c）。

用法：
    python3 gen_lvgl_icon.py <input.png> <output.c> <symbol_name> [size]

说明：
  - 画布按黑色底合成（启动器背景是纯黑，原图标同样是黑底 RGB565，无 alpha）。
  - 字节序为小端（低字节在前），与 LVGL LV_COLOR_FORMAT_RGB565 一致。
  - 生成文件头部与工程里既有的 icon_*.c 保持一致，可直接被 assets/assets.h 用
    LV_IMG_DECLARE(symbol_name) 声明。
"""

import os
import re
import sys

from PIL import Image

HEADER = """#ifdef __has_include
#if __has_include("lvgl.h")
#ifndef LV_LVGL_H_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE
#endif
#endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_{upper}
#define LV_ATTRIBUTE_IMAGE_{upper}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMAGE_{upper} uint8_t {symbol}_map[] = {{
"""

FOOTER = """}};

const lv_image_dsc_t {symbol} = {{
    .header.cf    = LV_COLOR_FORMAT_RGB565,
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.w     = {w},
    .header.h     = {h},
    .data_size    = {data_size},
    .data         = {symbol}_map,
}};
"""


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def convert(input_path: str, output_path: str, symbol: str, size: int):
    img = Image.open(input_path).convert("RGBA")
    if img.size != (size, size):
        img = img.resize((size, size), Image.LANCZOS)

    # 合成到黑底：LVGL RGB565 无 alpha 通道
    bg = Image.new("RGBA", (size, size), (0, 0, 0, 255))
    img = Image.alpha_composite(bg, img).convert("RGB")

    pixels = img.load()
    data = bytearray()
    for y in range(size):
        for x in range(size):
            r, g, b = pixels[x, y]
            v = rgb888_to_rgb565(r, g, b)
            data.append(v & 0xFF)  # 小端：低字节在前
            data.append((v >> 8) & 0xFF)

    upper = symbol.upper()
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(HEADER.format(upper=upper, symbol=symbol))
        for i in range(0, len(data), 16):
            chunk = data[i:i + 16]
            f.write("    " + " ".join(f"0x{b:02x}," for b in chunk) + "\n")
        f.write(FOOTER.format(symbol=symbol, w=size, h=size, data_size=len(data)))

    print(f"{output_path}: {size}x{size} RGB565, {len(data)} bytes data")


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)
    inp, out, symbol = sys.argv[1], sys.argv[2], sys.argv[3]
    size = int(sys.argv[4]) if len(sys.argv) > 4 else 200
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", symbol):
        print("symbol 必须是合法 C 标识符")
        sys.exit(1)
    convert(inp, out, symbol, size)


if __name__ == "__main__":
    main()
