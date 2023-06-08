# 手工制作：Mini-Retro-TV *by* `olonglongo`

## 目录

1. [文档](#文档)
2. [材料](#材料)
3. [步骤](#步骤)
   1. [第一步](#第一步)
   2. [第二步](#第二步)
   3. [第三步](#第三步)
4. [DIY](#DIY)

## 文档

- https://www.instructables.com/Mini-Retro-TV

## 材料

|材料|链接|价格(¥)|
|--|--|--|
|TTGO T7 V1.5Mini ESP32 开发板|[taobao](https://m.tb.cn/h.UxCFWjK?tk=iIrXdpDN4uP)|47.66|
|Micro SD卡槽(SDIOSIP)|[taobao](https://m.tb.cn/h.UxCEWh7?tk=cPRWdpDnkNx)|3.53|
|1.69寸圆角液晶分线板|[taobao](https://m.tb.cn/h.UCGLvon?tk=V4jjdpDNPVY)|23.62|
|MAX98357 I2S 音频分线板|[taobao](https://m.tb.cn/h.UDQaOOZ?tk=U3PAdpDnqJ2)|8.16|
|喇叭扬声器 3瓦4欧 3W 4R直径4CM|[taobao](https://m.tb.cn/h.UxCFsAl?tk=77qLdpDN2yL)|4.1|
|可选锂聚合物电池|/|15|
|一些平头 5 毫米长 M2 螺钉|/|2|
|大约 10 厘米长的母对母杜邦线|/|2|
|sd卡1g|/|10|
|3d打印模型|[thingiverse](https://cdn.thingiverse.com/tv-zip/5400343)|20|
|/|/|136.07|

## 步骤

### 第一步

软件环境配置

1. 下载并安装 Arduino IDE（如果您尚未安装）：
https://www.arduino.cc/en/main/software

2. ESP32支持
如果尚未添加 ESP32 支持，请按照 Arduino-ESP32[安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)进行操作。

3. Arduino_GFX 库
通过选择“工具”菜单 -> “管理库...”打开 Arduino IDE 库管理器。搜索“用于各种显示器的 GFX”，然后按“安装”按钮。
您可以参考[之前的教科书](https://www.instructables.com/ArduinoGFX/)以获取有关 Arduino_GFX 的更多信息。

4. JPEG解码器
通过选择“工具”菜单 -> “管理库...”打开 Arduino IDE 库管理器。搜索“JPEGDEC”并按“安装”按钮。

5. arduino-libhelix
该项目使用 Helix 解码器播放 AAC 或 MP3 音频。下载 arduino-libhelix 库并将其导入 Arduino IDE：
https://github.com/pschatzmann/arduino-libhelix.git

关于如何将库导入 Arduino IDE 的详细信息，您可以参考[Arduino文档](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries)


### 第二步 

烧录程序

1. 在 Arduino IDE 中打开“minitv.ino”
2. 连接 TTGO T7 开发板
3. 选择工具菜单 -> Board -> ESP32 Arduino -> ESP32 Dev Module
4. 选择工具菜单 -> PSRAM -> 禁用
5. 选择 Tools Menu -> Port -> [选择连接开发板的端口]
6. 按上传按钮
7. 等待编译上传成功

### 第三步 

硬件连接

![](media/16856849670214/16856868164991.jpg)

连接摘要：

```shell
TTGO T7 ESP32   ST7789 LCD   MAX98357 Audio   SD Card Slot
=============   ==========   ==============   ============
VCC          -> VCC       -> VCC           -> VCC
GND          -> GND       -> GND           -> GND
GPIO 4                                     -> MISO
GPIO 5       -> CS
GPIO 13                                    -> CS
GPIO 14                                    -> SCK
GPIO 15                                    -> MOSI
GPIO 18      -> CLK
GPIO 22      -> BLK
GPIO 23      -> SDA
GPIO 25                   -> BCLK/SCLK
GPIO 26                   -> LRCLK/LRCK
GPIO 27      -> DC
GPIO 32                   -> DOUT
GPIO 33      -> RST

```

## DIY
### Convert picture for SD card

```console
pip3 install -i https://pypi.doubanio.com/simple/ --trusted-host pypi.doubanio.com pillow
python3 media/image_conver.py ${image_file}
```

### Convert audio + video for SD card

#### MP3 Audio 44.1 kHz Mono

```console
ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 32k -filter:a loudnorm -filter:a "volume=-5dB" 44100.mp3
```

#### 288x240@30fps

```console
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_30fps.mjpeg
```