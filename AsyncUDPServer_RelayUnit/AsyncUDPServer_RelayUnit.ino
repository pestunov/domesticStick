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

#include "/home/pi/secure.h"
//#include "D:/1_Projects/secure.h"
const char * ssid = SECURE_SSID;
const char * password = SECURE_PASSWORD;

#define RELAY_1 16

boolean tag;
int command_parse_counter 

AsyncUDP udp;

void setup(){
  // initialize digital pins
  digitalWrite(RELAY_1, LOW);   // turn the port off
  pinMode(RELAY_1, OUTPUT);     // pin as output
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while(1){       // eternal loop. blink 1st led red
      delay(1000);
      if (tag) {
        tag = false;
        // do something
      } else {
        tag = true;
        // do something
      }
    }
  }
  
  if(udp.listen(3334)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      int len = packet.length(); int ii = 0;
      while (ii < len) {
        byte pixN = packet.data()[ii]; ii++;
      }
      //strip.Show();
    });
  }
}

void loop()
{
    delay(2000);
    //Send broadcast
    udp.broadcast("I'am #reley3");
}
