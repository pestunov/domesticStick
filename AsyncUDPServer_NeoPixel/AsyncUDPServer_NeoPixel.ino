/*
 * Base: ESP32
 * module intend for transport data 
 * in: UDP module --> out: WS2812B strip (thanks to Makuna)
 * 
 * received data format
 * 
 * (x*4)th byte - pixel num
 * (x*4)+1th byte - x pixel red brightness
 * (x*4)+2th byte - x pixel green brightness
 * (x*4)+3th byte - x pixel blue brightness
 * 
 * uses:
 * WiFi ver 1.2.7 by Arduino
 * NeoPixelBus by Makuna ver 2.6.0 by Michael C. Miller
 * 
 */


#include "WiFi.h"
#include "AsyncUDP.h"
#include <NeoPixelBus.h>

//global variables
boolean flag, tag;
int count = 0;

// strip setup
const uint16_t PixelCount = 226; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 4;  // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
RgbColor black(0);
RgbColor red(200,0,0);
RgbColor green(0,200,0);

// WiFi setup
const char * ssid = "Dimanche";
const char * password = "1234567899";

AsyncUDP udp;

void setup(){
  // this resets all the neopixels to an off state
  strip.Begin();
  strip.SetPixelColor(0, green);  // power on. 1st green.
                                  // wait for wifi connected
  for (int pixN = 1; pixN < PixelCount; pixN++) {
    strip.SetPixelColor(pixN, black);
  }
  strip.Show();

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while(1){       // eternal loop. blink 1st led red
      delay(1000);
      if (tag) {
        tag = false;
        strip.SetPixelColor(0, black);
        strip.Show();
      } else {
        tag = true;
        strip.SetPixelColor(0, red);  
        strip.Show();
      }
    }
  }
  
  if(udp.listen(3333)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      int len = packet.length(); int ii = 0;
      while (ii < len) {
        byte pixN = packet.data()[ii]; ii++;
        byte pixR = packet.data()[ii]; ii++;
        byte pixG = packet.data()[ii]; ii++;
        byte pixB = packet.data()[ii]; ii++;
        RgbColor myColor(pixR,pixG,pixB);
        strip.SetPixelColor(pixN, myColor);
      }
      strip.Show();
    });
  }
}

void loop()
{
    delay(2000);
    //Send broadcast
    udp.broadcast("who is here?");
}
