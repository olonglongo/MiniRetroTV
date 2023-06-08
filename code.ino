/*
   require libraries:
   https://github.com/moononournation/Arduino_GFX.git
   https://github.com/pschatzmann/arduino-libhelix.git
   https://github.com/bitbank2/JPEGDEC.git
*/
const char *jpeg_file = "/loading.jpg";


#define FPS 30
#define MJPEG_BUFFER_SIZE (288 * 240 * 2 / 10)
#define DOWN_BTN_PIN 35

#define AUDIOASSIGNCORE 1
#define DECODEASSIGNCORE 0
#define DRAWASSIGNCORE 1

#include <WiFi.h>
#include <SD.h>
#include <SD_MMC.h>

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
#define GFX_BL 22

#define TFT_DC 27
#define TFT_RST 33
#define TFT_CS 5    // only for displays with CS pin
#define TFT_MOSI 23 // for hardware SPI data pin (all of available pins)
#define TFT_SCLK 18 // for hardware SPI sclk pin (all of available pins)

#define SD_SCK 14
#define SD_MISO 4
#define SD_MOSI 15
#define SD_CS 13

#define I2S_MCLK -1
#define I2S_SCLK 25
#define I2S_LRCK 26
#define I2S_DOUT 32
#define I2S_DIN -1

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  TFT_DC /* DC */, TFT_CS /* CS */, TFT_SCLK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);
/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */

Arduino_GFX *gfx = new Arduino_ST7789(
  bus, TFT_RST /* RST */, 1 /* rotation */, true /* IPS */, 240 /* width */, 288 /* height */, 
  0 /* col offset 1 */, 20 /* row offset 1 */, 0 /* col offset 2 */, 12 /* row offset 2 */);

/* variables */
static int next_frame = 0;
static unsigned long start_ms, next_frame_ms;
static int8_t curr_video_idx = -1;
static bool btn_pressed = false;
static long last_pressed = -1;
static File aFile;
static File vFile;
static File root;

#define MAX_FILES 100
static String sdcard_files[MAX_FILES];
static String media_files[MAX_FILES];
static int sdcard_file_num = 0 ;
static int media_file_num = 0;


/* MP3 audio */
#include "esp32_audio.h"

/* MJPEG Video */
#include "MjpegClass.h"
static MjpegClass mjpeg;
static uint8_t *mjpeg_buf;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = (%d, %d), size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
} /* drawMCU() */


void IRAM_ATTR down_btn_pressed() {
	if ((millis() - last_pressed) >= 500) {
		last_pressed = millis();
		curr_video_idx++;
		if (curr_video_idx > media_file_num-1) {
			curr_video_idx = 0;
		}
		btn_pressed = true;
	}
}


void getFileList(File dir) {
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break; // No more files
    }

    String fileName = entry.name();

    if (entry.isDirectory()) {
      getFileList(entry); // Recursive call
    } else {
      if (fileName.endsWith(".mp3") || fileName.endsWith(".mjpeg")) {
        if (sdcard_file_num >= MAX_FILES){
          break;
        }
        sdcard_files[sdcard_file_num++] = "/"+fileName;
      }
    }

    entry.close();
  }
}

void getMediaList()
{
  root = SD.open("/");
  getFileList(root);
  root.close();
  
  for(int i=0; i<MAX_FILES; i++) {
    if (!sdcard_files[i].startsWith(".") && sdcard_files[i].endsWith(".mp3")) {
      // 文件以`.mp3`结尾，检查相同文件名的`.mjpeg`文件是否存在
      String fileName = sdcard_files[i].substring(0, sdcard_files[i].lastIndexOf('.')); // 去除`.mp3`扩展名

      // Check if file already exists in array
      bool found = false;
      for (size_t j = 0; j < MAX_FILES; j++) {
        if (sdcard_files[j].equals(fileName + ".mjpeg")) {
          found = true;
          break;
        }
      }
      // 存在相同文件名的`.mjpeg`文件，将它添加到`filtered_files`数组中
      if (found) {
        media_files[media_file_num++] = fileName;
      }
    }
  }

  Serial.printf("Found %d media files: \n" + media_file_num);
  // Print files in array
  for (size_t i = 0; i < media_file_num; i++) {
    Serial.println(media_files[i]);
  }
}


void setup()
{
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  // while (!Serial);

  // Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  // Init Audio
  esp_err_t ret_val = i2s_init(
    I2S_NUM_0, 44100, I2S_MCLK /* MCLK */, I2S_SCLK /* SCLK */, I2S_LRCK /* LRCK */, I2S_DOUT /* DOUT */, I2S_DIN /* DIN */);
  if (ret_val != ESP_OK)
  {
    Serial.printf("i2s_init failed: %d\n", ret_val);
  }
  i2s_zero_dma_buffer(I2S_NUM_0);

  SPIClass spi = SPIClass(HSPI);
  spi.begin(SD_SCK /* SCK */, SD_MISO /* MISO */, SD_MOSI /* MOSI */, SD_CS /* CS */);
  if (!SD.begin(SD_CS, spi, 80000000))
  {
    Serial.println(F("ERROR: File system mount failed!"));
  }
  else
  {
    mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
    if (!mjpeg_buf)
    {
      Serial.println(F("mjpeg_buf malloc failed!"));
    }
    else
    {
      File jFile = SD.open(jpeg_file);
      if (!jFile || jFile.isDirectory())
      {
        Serial.printf("ERROR: Failed to open %s file for reading\n", jpeg_file);
        gfx->printf("ERROR: Failed to open %s file for reading\n", jpeg_file);
      }
      else
      {
        // abuse mjpeg class display JPEG image
        mjpeg.setup(
          &jFile, mjpeg_buf, drawMCU, true /* useBigEndian */,0 /* x */, 0 /* y */, 
			    gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        mjpeg.readMjpegBuf();
        mjpeg.drawJpg();
        jFile.close();
        delay(3000);
        getMediaList();
        down_btn_pressed();
      }
      }
  }

  /* Init buttons */
  pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(DOWN_BTN_PIN, down_btn_pressed, FALLING);


  // btn_pressed=true;
  // if (btn_pressed)
  // {
  //   btn_pressed = false; // reset status
  //   curr_video_idx = 1;
  //   vFile = SD.open(mjpeg_files[curr_video_idx]);
  //   if (!vFile || vFile.isDirectory())
  //   {
  //       Serial.printf("ERROR: Failed to open %s file for reading\n", mjpeg_files[curr_video_idx]);
  //   }
  //   else
  //   {
  //       Serial.printf("Open MJPEG file %s.\n", mjpeg_files[curr_video_idx]);

  //       // Start playing MP3
  //       // BaseType_t ret_val = mp3_player_task_start(&aFile);
  //       // if (ret_val != pdPASS)
  //       // {
  //       //   Serial.printf("mp3_player_task_start failed: %d\n", ret_val);
  //       // }

  //       // Start playing MJPEG
  //       mjpeg.setup(
  //           &vFile, mjpeg_buf, drawMCU, true /* useBigEndian */,0 /* x */, 0 /* y */, 
  //           gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
  //       next_frame = 0;
  //       start_ms = millis();
  //       next_frame_ms = start_ms;
  //   }
  // }
  // for (;;){
  //   break;


  // if (curr_video_idx < 0)
  //   {
  //     delay(1000); // do nothing at the beginning
  //   }
  //   else if (millis() < next_frame_ms)
  //   {
  //     vTaskDelay(pdMS_TO_TICKS(1)); // wait for next frame
  //   }
  //   else
  //   {
  //     next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
  //     if (vFile.available() && mjpeg.readMjpegBuf()) // Read video
  //     {
  //       if (millis() < next_frame_ms) // check show frame or skip frame
  //       {
  //         // Play video
  //         mjpeg.drawJpg();
  //       }
  //       else
  //       {
  //         Serial.println(F("Skip frame"));
  //       }
  //     }
  //   }
  // }
  // btn_pressed=true;


  for (;;) {
   if (btn_pressed)
    {
      btn_pressed = false; // reset status
      Serial.println("Key Pressed.");

      // close previous files first
      if (vFile)
      {
        vFile.close();
      }

      if (aFile)
      {
        mp3_player_task_stop();
        aFile.close();
      }

      String mp3_file = media_files[curr_video_idx]+".mp3";
      String mjpeg_file = media_files[curr_video_idx]+".mjpeg";

      aFile = SD.open(mp3_file);
      if (!aFile || aFile.isDirectory())
      {
        Serial.printf("ERROR: Failed to open %s file for reading\n", mp3_file);
        gfx->printf("ERROR: Failed to open %s file for reading\n", mp3_file);
      }
      else
      {
        Serial.printf("Open MP3 file %s.\n", mp3_file);
        vFile = SD.open(mjpeg_file);
        if (!vFile || vFile.isDirectory())
        {
          Serial.printf("ERROR: Failed to open %s file for reading\n", mjpeg_file);
          gfx->printf("ERROR: Failed to open %s file for reading\n", mjpeg_file);
        }
        else
        {
          Serial.printf("Open MJPEG file %s.\n", mjpeg_file);

          // Start playing MP3
          BaseType_t ret_val = mp3_player_task_start(&aFile);
          if (ret_val != pdPASS)
          {
            Serial.printf("mp3_player_task_start failed: %d\n", ret_val);
          }

          // Start playing MJPEG
          mjpeg.setup(
              &vFile, mjpeg_buf, drawMCU, true /* useBigEndian */,
              0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
          next_frame = 0;
          start_ms = millis();
          next_frame_ms = start_ms;
        }
      }
    }


    if (curr_video_idx < 0)
      {
        delay(1000); // do nothing at the beginning
      }
      else if (millis() < next_frame_ms)
      {
        vTaskDelay(pdMS_TO_TICKS(1)); // wait for next frame
      }
      else
      {
        next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
        if (vFile.available() && mjpeg.readMjpegBuf()) // Read video
        {
          if (millis() < next_frame_ms) // check show frame or skip frame
          {
            // Play video
            mjpeg.drawJpg();
          }
          else
          {
            Serial.println(F("Skip frame"));
          }
        }else{
          down_btn_pressed();
        }
      }
   }
}

void loop() {}
