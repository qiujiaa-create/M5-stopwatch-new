#!/usr/bin/env python3
"""
绘制 StopWatch 启动器的「Xiaozhi」图标（200x200 PNG，透明底）。

风格对齐工程里既有的 icon_*.c：黑底上的一枚玻璃质感彩色 App 磁贴 + 外发光。
生成后用 tools/gen_lvgl_icon.py 转成 LVGL RGB565 的 C 数组。

用法：
    python3 draw_xiaozhi_icon.py            # 输出到 ../../assets-src/icon_xiaozhi.png
    python3 draw_xiaozhi_icon.py out.png
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFilter

SIZE = 200          # 最终尺寸
SS = 4              # 超采样倍数
S = SIZE * SS       # 画布尺寸

# 主体渐变：蓝 -> violet
C_TOP = (74, 130, 255)
C_BOTTOM = (150, 88, 245)
GLOW = (86, 140, 255)

TILE_RADIUS = int(52 * SS)


def squircle_mask(size: int, radius: int) -> Image.Image:
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    return m


def linear_gradient(size: int, c0, c1) -> Image.Image:
    """对角线性渐变"""
    g = Image.new("RGB", (size, size))
    px = g.load()
    for y in range(size):
        for x in range(size):
            t = (x + y) / (2 * (size - 1))
            px[x, y] = (
                int(c0[0] + (c1[0] - c0[0]) * t),
                int(c0[1] + (c1[1] - c0[1]) * t),
                int(c0[2] + (c1[2] - c0[2]) * t),
            )
    return g


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "assets-src", "icon_xiaozhi.png"
    )

    canvas = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    mask = squircle_mask(S, TILE_RADIUS)

    # --- 1. 外发光：把磁贴形状模糊后铺在底层 ---
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    glow.paste(GLOW + (255,), (0, 0), mask)
    glow = glow.filter(ImageFilter.GaussianBlur(radius=26 * SS))
    a = glow.getchannel("A").point(lambda v: int(v * 0.62))
    glow.putalpha(a)
    canvas = Image.alpha_composite(canvas, glow)

    # --- 2. 磁贴本体：渐变 + 内描边高光 ---
    tile = linear_gradient(S, C_TOP, C_BOTTOM).convert("RGBA")
    canvas.paste(tile, (0, 0), mask)

    # 顶部柔和高光，制造玻璃感
    hl = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    hd = ImageDraw.Draw(hl)
    hd.ellipse(
        [-int(0.15 * S), -int(0.62 * S), int(1.15 * S), int(0.55 * S)],
        fill=(255, 255, 255, 64),
    )
    hl = hl.filter(ImageFilter.GaussianBlur(radius=22 * SS))
    hl.putalpha(Image.composite(hl.getchannel("A"), Image.new("L", (S, S), 0), mask))
    canvas = Image.alpha_composite(canvas, hl)

    # 内描边（一圈淡淡的亮边）
    edge = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ed = ImageDraw.Draw(edge)
    ed.rounded_rectangle(
        [int(3 * SS), int(3 * SS), S - 1 - int(3 * SS), S - 1 - int(3 * SS)],
        radius=TILE_RADIUS,
        outline=(255, 255, 255, 70),
        width=int(2 * SS),
    )
    edge = edge.filter(ImageFilter.GaussianBlur(radius=1.5 * SS))
    canvas = Image.alpha_composite(canvas, edge)

    # --- 3. 白色麦克风字形 ---
    glyph = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glyph)

    cx = S // 2

    # 胶囊（话筒头）
    cap_w, cap_h = int(34 * SS), int(66 * SS)
    cap_top = int(46 * SS)
    gd.rounded_rectangle(
        [cx - cap_w // 2, cap_top, cx + cap_w // 2, cap_top + cap_h],
        radius=cap_w // 2,
        fill=(255, 255, 255, 255),
    )

    # 下方的托架圆弧
    arc_w = int(9 * SS)
    arc_r = int(32 * SS)
    arc_cy = cap_top + cap_h - int(6 * SS)
    gd.arc(
        [cx - arc_r, arc_cy - arc_r, cx + arc_r, arc_cy + arc_r],
        start=0,
        end=180,
        fill=(255, 255, 255, 255),
        width=arc_w,
    )

    # 支架与底座
    stem_w = int(9 * SS)
    stem_top = arc_cy + arc_r - arc_w // 2 + int(2 * SS)
    stem_h = int(14 * SS)
    gd.rounded_rectangle(
        [cx - stem_w // 2, stem_top, cx + stem_w // 2, stem_top + stem_h],
        radius=stem_w // 2,
        fill=(255, 255, 255, 255),
    )
    bar_w, bar_h = int(38 * SS), int(9 * SS)
    bar_top = stem_top + stem_h - bar_h // 2
    gd.rounded_rectangle(
        [cx - bar_w // 2, bar_top, cx + bar_w // 2, bar_top + bar_h],
        radius=bar_h // 2,
        fill=(255, 255, 255, 255),
    )

    # 字形下方投影，和其余图标一样有轻微立体感
    blur = glyph.filter(ImageFilter.GaussianBlur(radius=6 * SS))
    alpha = blur.getchannel("A").point(lambda v: int(v * 0.35))
    dark = Image.new("RGBA", (S, S), (30, 20, 60, 255))
    dark.putalpha(alpha)
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shadow.paste(dark, (0, int(3 * SS)))
    canvas = Image.alpha_composite(canvas, shadow)
    canvas = Image.alpha_composite(canvas, glyph)

    # --- 输出 ---
    final = canvas.resize((SIZE, SIZE), Image.LANCZOS)
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    final.save(out)
    print(f"saved {out} ({SIZE}x{SIZE})")


if __name__ == "__main__":
    main()
