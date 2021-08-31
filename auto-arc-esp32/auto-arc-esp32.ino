#include <M5StickC.h>
#include "arc_manager.h"
#include <HTTPClient.h>

static constexpr const uint32_t UPDATE_INTERVAL_MS = 60000;

void setup()
{
  Serial.begin(115200);
  Serial.println("M5StickC started.");
  M5.begin();
  setupArc();
}

void loop()
{
    WiFiClient client;

    if( !client.connect("metadata.soracom.io", 80) ) {
        Serial.println("Failed to connect...");
        delay(5000);
        return;
    }
    
    client.write("GET /v1/subscriber.imsi HTTP/1.1\r\n");
    client.write("Host: metadata.soracom.io\r\n");
    client.write("\r\n\r\n");

    while(client.connected()) {
        auto line = client.readStringUntil('\n');
        Serial.write(line.c_str());
        Serial.write("\n");
        if( line == "\r" ) break;
    }
    if(client.connected()) {
        uint8_t buffer[256];
        size_t bytesToRead = 0;
        while((bytesToRead = client.available()) > 0) {
            bytesToRead = bytesToRead > sizeof(buffer) ? sizeof(buffer) : bytesToRead;
            auto bytesRead = client.readBytes(buffer, bytesToRead); 
            Serial.write(buffer, bytesRead);
        }
    }
    delay(UPDATE_INTERVAL_MS);
}
