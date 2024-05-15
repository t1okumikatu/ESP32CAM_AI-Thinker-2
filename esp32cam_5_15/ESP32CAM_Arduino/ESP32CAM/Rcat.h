struct WiFitype {
  char ssid[32];
  char pass[64];
  char ssid2[32];
  char pass2[64];
};
struct FixedIPtype {
  byte Flag;
  unsigned long IP;
  unsigned long MASK;
  unsigned long GATE;
};
void SaveWiFiConf();
void SaveFixedIPConf();