import os
import sys
from PIL import Image, ImageFont, ImageDraw

def generate_font():
    font_path = "C:\\Windows\\Fonts\\msyh.ttc"  # 微软雅黑
    if not os.path.exists(font_path):
        font_path = "C:\\Windows\\Fonts\\simhei.ttf" # 备用黑体
        if not os.path.exists(font_path):
            print("找不到默认字体，请修改 font_path！")
            return

    font_size = 24
    font = ImageFont.truetype(font_path, font_size)

    # 常用汉字 (3500 个常用字 + 常用标点)
    # 为避免脚本太长，这里我们先抓取 GB2312 的第一级常用汉字（3755个）
    # 并且加上 ASCII 字符 32-126
    chars = []
    # ASCII
    for i in range(32, 127):
        chars.append(chr(i))
    
    # 中文标点
    zh_punc = "，。！？；：“”‘’【】《》、·\n…—"
    for c in zh_punc:
        chars.append(c)

    # 完整包含所有常用汉字 (CJK Unified Ideographs: 4E00 - 9FFF，约两万个字)
    # 这几乎涵盖了所有的中文小说用字！
    for u in range(0x4E00, 0x9FA6):
        chars.append(chr(u))

    # 除了内置的标点，我们也扫一遍 app_ebook.c，确保测试文本里所有的字都在字库中！
    try:
        with open("main/apps/app_ebook/app_ebook.c", "r", encoding="utf-8") as f:
            c_code = f.read()
            for ch in c_code:
                # 只有汉字和标点才加进去 (防止加了一堆乱码)
                if ord(ch) >= 32:
                    chars.append(ch)
    except Exception as e:
        print("无法读取 app_ebook.c，只使用默认字符。错误:", e)

    # 按 Unicode 排序以便于 C 语言里做二分查找
    chars.sort(key=lambda x: ord(x))
    chars = list(dict.fromkeys(chars)) # 去重

    print(f"准备生成 {len(chars)} 个字符的点阵字库...")

    out_file = "my_chinese_font.h"
    with open(out_file, "w", encoding="utf-8") as f:
        f.write("// 自定义 24x24 紧凑型点阵字库\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define NUM_CHARS {len(chars)}\n\n")
        
        f.write("typedef struct {\n")
        f.write("    uint16_t unicode;\n")
        f.write("    uint8_t bitmap[72]; // 24x24 pixels = 576 bits = 72 bytes\n")
        f.write("} CustomGlyph;\n\n")
        
        f.write("const CustomGlyph my_font[] = {\n")
        
        for c in chars:
            img = Image.new('1', (24, 24), color=0)
            draw = ImageDraw.Draw(img)
            
            # 使用 textbbox 获取边界
            bbox = draw.textbbox((0, 0), c, font=font)
            if bbox is not None:
                w = bbox[2] - bbox[0]
                h = bbox[3] - bbox[1]
                x = (24 - w) / 2 - bbox[0]
                y = (24 - h) / 2 - bbox[1]
            else:
                x, y = 0, 0

            draw.text((x, y), c, font=font, fill=1)
            
            pixels = img.load()
            bitmap = []
            for row in range(24):
                byte_val = 0
                for col in range(24):
                    if pixels[col, row] > 0:
                        byte_val |= (1 << (7 - (col % 8)))
                    if col % 8 == 7:
                        bitmap.append(byte_val)
                        byte_val = 0
            
            # 转为 hex 字符串
            hex_str = ", ".join([f"0x{b:02X}" for b in bitmap])
            
            # 安全处理换行符等特殊字符的注释
            if c == '\n':
                display_char = r"'\n'"
            elif c == '\r':
                display_char = r"'\r'"
            else:
                display_char = f"'{c}'"
                
            f.write(f"    {{ 0x{ord(c):04X}, {{ {hex_str} }} }}, // {display_char}\n")

        f.write("};\n")
    print(f"字库生成成功！保存在 {out_file}，大小大约为 {len(chars) * 74 / 1024:.1f} KB")

if __name__ == "__main__":
    generate_font()