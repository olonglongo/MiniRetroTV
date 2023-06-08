# -*- coding=utf-8 -*-

from PIL import Image
import argparse

# 命令行输入参数处理
parser = argparse.ArgumentParser()

parser.add_argument('file')  # 输入文件
parser.add_argument('-o', '--output', default="output.jpg")  # 输出文件
parser.add_argument('--width', type=int, default=288)  # 输出字符画宽
parser.add_argument('--height', type=int, default=240)  # 输出字符画高

# 获取参数
args = parser.parse_args()

IMG = args.file
WIDTH = args.width
HEIGHT = args.height
OUTPUT = args.output

if __name__ == '__main__':
    im = Image.open(IMG)
    im = im.resize((WIDTH, HEIGHT), Image.NEAREST)
    im.save(OUTPUT)
