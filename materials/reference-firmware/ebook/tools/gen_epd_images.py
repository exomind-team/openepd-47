#!python3
"""
批量把多张图片 (bmp/jpg/png) 转成 .h 头文件，供 epdiy dragon 示例使用。

功能说明：
1. 自动扫描指定目录（默认：工程根目录下 assets/）
2. 支持扩展名：.bmp, .dib, .jpg, .jpeg, .png（不区分大小写）
3. 最多转换 MAX_IMAGES_TO_CONVERT 张图片（当前配置为 5）
4. 不要求源文件名规则，按文件名字典序排序后依次映射为：
   epd_img1, epd_img2, epd_img3, epd_img4, epd_img5

每个 .h 文件的内容类似：
    #pragma once
    #include <stdint.h>
    #define EPD_IMG1_DEFINED 1
    const uint32_t epd_img1_width = ...;
    const uint32_t epd_img1_height = ...;
    const uint8_t  epd_img1_data[...] = {...};

在 C 代码中可以通过：
    #include "epd_img1.h"
    #ifdef EPD_IMG1_DEFINED
      ...
    #endif
来判断这张图片是否存在。

运行方式（从工程根目录）：
    python tools/gen_epd_images.py
"""

import math
import sys
from pathlib import Path
from typing import List

from PIL import Image

# ---------------- 配置区（必要时你可以改这里） ----------------

# 最大转换图片数量（按文件名排序后取前 N 个）
MAX_IMAGES_TO_CONVERT = 5

# 预设名字（符号名前缀），最多支持 10 个
# 当前脚本只会用到前 MAX_IMAGES_TO_CONVERT 个
PRESET_SYMBOL_NAMES: List[str] = [
    "epd_img1",
    "epd_img2",
    "epd_img3",
    "epd_img4",
    "epd_img5",
    "epd_img6",
    "epd_img7",
    "epd_img8",
    "epd_img9",
    "epd_img10",
]

# 单张图片的最大宽高（超过会等比例缩小）
MAX_WIDTH = 1872
MAX_HEIGHT = 1404

# 支持的图片扩展名
IMAGE_EXTS = {".bmp", ".dib", ".jpg", ".jpeg", ".png"}

# 源图片目录（相对于工程根目录）
INPUT_DIR_REL = "pictures"

# 头文件输出目录（相对于工程根目录）
OUTPUT_DIR_REL = "components/epdiy/include"

# ---------------------------------------------------------


def find_project_root() -> Path:
    """
    简单假定：本脚本位于 <工程根>/tools 下，
    所以工程根 = 当前脚本路径的上一级的上一级。
    """
    return Path(__file__).resolve().parent.parent


def scan_image_files(input_dir: Path) -> List[Path]:
    """
    从 input_dir 中扫描支持的图片文件，按文件名排序。
    """
    if not input_dir.exists():
        print(f"[ERROR] 图片目录不存在: {input_dir}")
        return []

    files = []
    for p in input_dir.iterdir():
        if p.is_file() and p.suffix.lower() in IMAGE_EXTS:
            files.append(p)

    files.sort(key=lambda p: p.name.lower())
    return files


def convert_single_image_to_header(
    img_path: Path,
    symbol_name: str,
    output_header: Path,
):
    """
    把单张图片转换成 4bit 灰度的 C 头文件：
      const uint32_t <symbol_name>_width;
      const uint32_t <symbol_name>_height;
      const uint8_t  <symbol_name>_data[...]  // 每个像素 4bit，两像素打包成一字节
    并额外定义：
      #define <SYMBOL_MACRO>_DEFINED 1
    例如 symbol_name = "epd_img1" 时，会生成：
      #define EPD_IMG1_DEFINED 1
    """
    print(f"[INFO] 转换图片: {img_path} → {output_header}  (symbol={symbol_name})")

    img = Image.open(img_path)

    # 转成灰度图 (L 模式, 0..255)
    img = img.convert(mode="L")

    # 限制最大宽高（等比例缩放）
    img.thumbnail((MAX_WIDTH, MAX_HEIGHT), Image.LANCZOS)

    width, height = img.size
    # 按 epdiy 脚本的写法，数组长度使用 ceil(width / 2) * 2 * height / 2
    width_rounded_even = math.ceil(width / 2) * 2
    total_bytes = (width_rounded_even * height) // 2

    # 确保输出目录存在
    output_header.parent.mkdir(parents=True, exist_ok=True)

    macro_name = symbol_name.upper()  # epd_img1 → EPD_IMG1

    with open(output_header, "w", encoding="utf-8") as f:
        f.write("// 自动生成文件，请不要手工修改。\n")
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")

        f.write(f"#define {macro_name}_DEFINED 1\n\n")

        f.write(f"const uint32_t {symbol_name}_width = {width};\n")
        f.write(f"const uint32_t {symbol_name}_height = {height};\n")
        f.write(
            f"const uint8_t {symbol_name}_data[{total_bytes}] = {{\n"
        )

        for y in range(height):
            byte = 0
            done = True
            f.write("    ")
            for x in range(width):
                l = img.getpixel((x, y))  # 0..255

                if x % 2 == 0:
                    # 高 4bit
                    byte = (l >> 4) & 0x0F
                    done = False
                else:
                    # 低 4bit（用高 4bit 放到低位）
                    byte |= (l & 0xF0)
                    f.write(f"0x{byte:02X}, ")
                    done = True

            # 如果本行宽度是奇数，最后还剩一个 nibble，没有和第二个像素配对
            if not done:
                # 直接把高4bit写入（低4bit 自动为 0）
                f.write(f"0x{byte:02X}, ")
            f.write("\n")

        f.write("};\n")

    print(f"[OK] 生成头文件: {output_header}  (w={width}, h={height}, bytes={total_bytes})")


def main():
    project_root = find_project_root()
    input_dir = project_root / INPUT_DIR_REL
    output_dir = project_root / OUTPUT_DIR_REL

    if MAX_IMAGES_TO_CONVERT > len(PRESET_SYMBOL_NAMES):
        print("[ERROR] MAX_IMAGES_TO_CONVERT 大于预设符号数量，请先扩展 PRESET_SYMBOL_NAMES。")
        sys.exit(1)

    print(f"[INFO] 工程根目录: {project_root}")
    print(f"[INFO] 图片目录: {input_dir}")
    print(f"[INFO] 头文件输出目录: {output_dir}")
    print(f"[INFO] 最大转换图片数量: {MAX_IMAGES_TO_CONVERT}")

    files = scan_image_files(input_dir)
    if not files:
        print("[WARN] 未在图片目录中找到任何支持的图片文件。")
        return

    selected_files = files[:MAX_IMAGES_TO_CONVERT]
    print("[INFO] 将要转换的图片：")
    for idx, p in enumerate(selected_files):
        print(f"  [{idx}] {p.name}")

    for idx, img_path in enumerate(selected_files):
        symbol = PRESET_SYMBOL_NAMES[idx]  # epd_img1, epd_img2, ...
        header_name = f"{symbol}.h"
        out_path = output_dir / header_name

        convert_single_image_to_header(
            img_path=img_path,
            symbol_name=symbol,
            output_header=out_path,
        )

    print("[DONE] 所有图片已处理完毕。")


if __name__ == "__main__":
    main()
