/*
CameraWebServer (Rcat999加筆版)

ESP32CAM用のサンプルスケッチにRcat999が加筆した強化版

利用規約
https://note.com/rcat999/n/nb6a601a36ef5

###強化点###
・Wi-Fi設定を後から行う
スケッチに設定を書かないので、SSID/PASSが変わってもスケッチを書き換えなくて良いです

・固定IPが利用可能
上記Wi-Fi設定画面で固定IPアドレスの使用に関する設定も可能

・GoogleDriveオンラインOTAアップデート機能搭載
Rcat999開発のGoogleドライブからバイナリをダウンロードしてアップデートする機能を搭載
ファームウェアを無線で書き換えられます


変更点は基本的にコメントで記述済み
*/

#include "esp_camera.h"
#include <WiFi.h>

//ファイルシステムエクスプローラ/コンフィグ
#include <SPIFFS.h>
#include <FS.h>

/*
ESP32 Wrover Module
80MHz
QIO
HugeAPP(3MB NoOTA/1MB)
*/
//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER  // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"


//変更点
bool UpdateFlag = false;
String UpdateURL = "";


//Wi-Fiタブ用
struct WiFitype {
  char ssid[32];
  char pass[64];
  char ssid2[32];
  char pass2[64];
};
struct WiFitype wifi;

struct FixedIPtype {
  byte Flag;
  unsigned long IP;
  unsigned long MASK;
  unsigned long GATE;
};
struct FixedIPtype FixedIP;


void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_SVGA);  //変更点

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  //変更----------------------------------------------
  Serial.println("SPIFFSinit");
  SPIFFS.begin(true);
  Serial.println("LoadConf");
  LoadWiFiConf();
  ReadFixedIP();
  //WiFi.begin(ssid, password);
  WiFi.begin(wifi.ssid, wifi.pass);
  long WiFiTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > WiFiTime + 10000) break;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("RcatESP", "12345678");
    const IPAddress ip(192, 168, 0, 1);
    const IPAddress gateway(192, 168, 0, 1);
    const IPAddress netmask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, netmask);
  }
  //変更----------------------------------------------

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  pinMode(4, OUTPUT);  //LEDライト追加分
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  if (UpdateFlag) {
    OnlineUpdate();
  }
}
