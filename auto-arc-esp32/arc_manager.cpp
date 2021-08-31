#include <M5StickC.h>
#include <EEPROM.h>

#include "arc_manager.h"

#define WM_DEBUG_LEVEL 4

#include <WiFiManager.h>
WiFiManager wifiManager;

class IPAddressParameter : public WiFiManagerParameter {
public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip) {
        return ip.fromString(WiFiManagerParameter::getValue());
    }
};

class IntParameter : public WiFiManagerParameter {
public:
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }
};

struct Settings {
    char private_key[64];
    char public_key[64];
    char endpoint_address[32];
    uint32_t local_ip;
    uint16_t endpoint_port;
} sett;

#include <WiFi.h>
#include <WireGuard-ESP32.h>
static WireGuard wg;

// WiFi設定が成功したかどうかのフラグ
bool isWifiConfigSucceeded = false;

// LCD画面にメッセージを表示する
void showMessage(String msg)
{
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println(msg);
}

// WiFi接続モードに移行した時に呼ばれるコールバック
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  showMessage("Please connect to this AP\nto config WiFi\nSSID: " + myWiFiManager->getConfigPortalSSID());
}

// 起動後すぐにパワーボタンが押されたらWiFi設定モードに移行し、そうでなければ自動接続を行う
void setupArc()
{
  EEPROM.begin( 512 );
  EEPROM.get(0, sett);
  Serial.println("Settings loaded");

  sett.private_key[63]='\0';
  sett.public_key[63]='\0';
  sett.endpoint_address[63]='\0';

  IPAddress ip(sett.local_ip);
  IPAddressParameter local_ip("local_ip", "interface address", ip);
  
  WiFiManagerParameter private_key("private_key", "interface private key", sett.private_key, 64);
  WiFiManagerParameter public_key("public_key", "peer public key", sett.public_key, 64);
  WiFiManagerParameter endpoint_address("endpoint_address", "peer endpoint address", sett.endpoint_address, 32);
  IntParameter endpoint_port("endpoint_port", "peer endpoint port", sett.endpoint_port, 6);
  
  wifiManager.addParameter(&private_key);
  wifiManager.addParameter(&local_ip);
  wifiManager.addParameter(&public_key);
  wifiManager.addParameter(&endpoint_address);
  wifiManager.addParameter(&endpoint_port);
  wifiManager.setAPCallback(configModeCallback);
 
  // clicking power button at boot time to enter wifi config mode
  bool doManualConfig = false;
  showMessage("Push POWER button to enter Wifi config.");
  for(int i=0 ; i<200 ; i++) {
    M5.update();
    if (M5.Axp.GetBtnPress()) {
      doManualConfig = true;
      break;
    }
    delay(10);
  }

  if (doManualConfig) {
    Serial.println("wifiManager.startConfigPortal()");
    if (wifiManager.startConfigPortal()) {
      isWifiConfigSucceeded = true;
      Serial.println("startConfigPortal() connect success!");
    }
    else {
      Serial.println("startConfigPortal() connect failed!");
    }
  } else {
    showMessage("Wi-Fi connecting...");

    Serial.println("wifiManager.autoConnect()");
    if (wifiManager.autoConnect()) {
      isWifiConfigSucceeded = true;
      Serial.println("autoConnect() connect success!");
    }
    else {
      Serial.println("autoConnect() connect failed!");
    }
  }
  if (!isWifiConfigSucceeded)
  {
    showMessage("Wi-Fi failed.");
    for(;;);    
  }
  showMessage("Wi-Fi connected.");

  strncpy(sett.private_key, private_key.getValue(), 64);
  sett.private_key[63]='\0';
  strncpy(sett.public_key, public_key.getValue(), 64);
  sett.public_key[63]='\0';
  strncpy(sett.endpoint_address, endpoint_address.getValue(), 32);
  sett.endpoint_address[31]='\0';
  sett.endpoint_port = endpoint_port.getValue();
  if (local_ip.getValue(ip)) {
    sett.local_ip = ip;
    Serial.print("Local IP: ");
    Serial.println(ip); 
  } else {
    Serial.println("Incorrect IP");
  }
  EEPROM.put(0, sett);
  if (EEPROM.commit()) {
      Serial.println("Settings saved");
  } else {
      Serial.println("EEPROM error");
  }
          
  showMessage("Adjusting system time...");
  configTime(9 * 60 * 60, 0, "ntp.jst.mfeed.ad.jp", "ntp.nict.jp", "time.google.com");
  Serial.println(private_key.getValue());
  Serial.println(endpoint_address.getValue());
  Serial.println(public_key.getValue());
  Serial.println(endpoint_port.getValue());
  wg.begin(
      ip,
      private_key.getValue(),
      endpoint_address.getValue(),
      public_key.getValue(),
      (uint16_t) endpoint_port.getValue()
  );
  showMessage("WireGuard started.");
}
