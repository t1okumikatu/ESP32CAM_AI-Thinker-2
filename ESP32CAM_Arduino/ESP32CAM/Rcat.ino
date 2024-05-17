/*
Google Drive オンラインOTAアップデート用関数
(C) 2023 Rcat999
http://192.168.0.136/update?var=id&val=15izVVohVk2tF8tfituOVki88LiTHJgiE
*/
//OTAアップデート関連 
#include <Update.h>
#include <HTTPClient.h>






//##############################################################
bool LoadWiFiConf() {
  //struct WiFitype wifi; グローバルで宣言済み
  File file = SPIFFS.open("/wifi.ini", FILE_READ);
  if (!file) return false;
  if (file.size() <= 0) return false;
  file.read((uint8_t *)&wifi, sizeof(wifi));
  file.close();
  return true;
}
//-------------------------------------------------------------------
void SaveWiFiConf() {
  File file = SPIFFS.open("/wifi.ini", FILE_WRITE);
  file.write((uint8_t *)&wifi, sizeof(wifi));
  file.close();
}

//##############################################################
bool LoadFixedIPConf() {
  //struct FixedIPtype FixedIP; グローバルで宣言済み
  File file = SPIFFS.open("/FixedIP.ini", FILE_READ);
  if (!file) return false;
  if (file.size() <= 0) return false;
  file.read((uint8_t *)&FixedIP, sizeof(FixedIP));
  file.close();
  return true;
}
//-------------------------------------------------------------------
void SaveFixedIPConf() {
  File file = SPIFFS.open("/FixedIP.ini", FILE_WRITE);
  file.write((uint8_t *)&FixedIP, sizeof(FixedIP));
  file.close();
}

void ReadFixedIP() {
  LoadFixedIPConf();
  if (FixedIP.Flag == 2) {
    //Serial.println(F("USE Fixed IP"));
    byte ip[4];
    byte gate[4];
    byte mask[4];
    LongToBytes(FixedIP.IP, ip);
    LongToBytes(FixedIP.MASK, mask);
    LongToBytes(FixedIP.GATE, gate);
    const IPAddress fip(ip[0], ip[1], ip[2], ip[3]);
    const IPAddress fgateway(mask[0], mask[1], mask[2], mask[3]);
    const IPAddress fmask(gate[0], gate[1], gate[2], gate[3]);
    Serial.println(fip);
    Serial.println(fgateway);
    Serial.println(fmask);
    if (!WiFi.config(fip, fgateway, fmask)) Serial.print(F("IP fixing failed"));
  }
}
void SaveFixedIP(byte Flag, unsigned long IP, unsigned long MASK, unsigned long GATE) {
  //struct FixedIPtype Fixedip;
  FixedIP.Flag = Flag;
  FixedIP.IP = IP;
  FixedIP.MASK = MASK;
  FixedIP.GATE = GATE;
  SaveFixedIPConf();
}

//Long型をByte[4]に変換する
void LongToBytes(long Val, byte buf[4]) {
  for (int i = 0; i < 4; i++) {
    buf[3 - i] = (Val >> 8 * i) & 255;
  }
}


//########################################################################################################################
void OnlineUpdate() {
  Serial.println("Acces Google Drive");
  HTTPClient http;
  http.begin(UpdateURL);

  int httpCode = 0;
  unsigned long count = 0;
  unsigned long Lastgettime = millis();
  String newUrl;
  httpCode = http.GET();
  //Googleドライブは直リンクでもリダイレクトしてくる対策
  while (httpCode == 303) {
    newUrl = http.getLocation();
    http.end();
    http.begin(newUrl);
    httpCode = http.GET();
  }

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient *stream = http.getStreamPtr();
    Update.begin();
    Serial.println("Start Update");
    while (stream->available()) {
      uint8_t bdata = stream->read();
      count++;
      Update.write(&bdata, 1);
      if(count % 100 == 0) Serial.println(count);
      Lastgettime = millis();
      while (!stream->available() && millis() < Lastgettime + 1000)
        ;
    }
    Update.end(true);
    Serial.println("End Update");
  } else {
    Serial.println("Err");
    return;
  }
  http.end();
  Serial.println("Reboot");
  ESP.restart();
}
