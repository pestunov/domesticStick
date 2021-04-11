/*
 * Base: ESP32
 * dome access controller
 * uses: rfid RC522
 * 
    RC-522 | Arduino 
    ----------------
       SDA | D
       SCK | D
      MOSI | D
      MISO | D
       IRQ | N/A
       GND | GND
       RST | D
      3.3V | 3.3V  
 * uses:
 * WiFi ver 1.2.7 by Arduino
 * 
 * 
 */

/*** start common WiFi UDP block */
#include <WiFi.h>
#include <WiFiUdp.h>
#include "/home/pi/secure.h"
//#include "D:/1_Projects/secure.h"

#define CYCLE_PERIOD 200
#define UNIT_NAME "UNIT000003"
#define UDP_TX_PACKET_MAX_SIZE 1000
#define UDP_TX_PACKET_HEAD_SIZE 40

String unitName; 
int unitId = 1; 

const char* ssid = SECURE_SSID;          //secure.h #define SECURE_SSID = "my_ssid12345"
const char* password = SECURE_PASSWORD;  //secure.h 
const char* udpAddress = "192.168.001.255"; // a network broadcast address
const int udpPort = 3333;

enum {
  JUST_CONNECTED,
  CONNECTED,
  DISCONNECTED,
  DISCON_TRIED
  } wifiState;

enum {NOP, SEND_STATUS, SET_KEY, SET_DATA, GET_KEY, GET_DATA} txCommand;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; // buffer to hold incoming packet
char packetBufferTerminator = 0;

int parseCounter;
int cycle_counter = 0;
char my_status, my_tasks, parsePort, parseState;
boolean tag = false;

WiFiUDP udp; // udp library class
/*** end of common WiFi UDP block */

// strip setup
const uint16_t PixelNum = 226;  //226 this example assumes more then 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 4;       // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelNum, PixelPin);
RgbColor black(0);
RgbColor red(200,0,0);
RgbColor pale_red(1,0,0);
RgbColor green(0,200,0);
RgbColor pale_green(0,1,0);


void setup(){
  Serial.begin(115200);
  setupCommon();
  // Switch off all of the neopixels
  strip.Begin();
  strip.SetPixelColor(0, pale_green);  // power on. 1st green.
  for (int pixN = 1; pixN < PixelNum; pixN++) {
    strip.SetPixelColor(pixN, black);
  }
  strip.Show();
}

void loop()
{
  for(int cycle_counter = 0; cycle_counter < CYCLE_PERIOD; cycle_counter++){
    delay(10);
    if ((cycle_counter == 10) && (wifiState == JUST_CONNECTED)){ //only send data when connected
      wifiState = CONNECTED;
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.print(unitName);
      udp.print(" just connected");
      udp.endPacket();

      tag = true;
      strip.SetPixelColor(0, green);
      strip.Show();

    }
    if ((cycle_counter == 12) && (wifiState == CONNECTED)){ //only send data when connected
      udp.beginPacket(udpAddress,udpPort);
      udp.print(unitName);
      udp.printf(" still here %lu", millis()/1000);
      udp.endPacket();
    }
    if ((cycle_counter == 14) && (wifiState == DISCON_TRIED)){ // try to reconnect
      wifiState = DISCONNECTED;
      connectToWiFi(ssid, password);
      tag = true;
      strip.SetPixelColor(0, red);
      strip.Show();
    }
    if ((cycle_counter == 16) && (wifiState == DISCONNECTED)){ // still disconnected
      Serial.print('.');
      tag = true;
      strip.SetPixelColor(0, pale_red);
      strip.Show();
    }
    if ((cycle_counter == 20) && (tag)){
      tag = false;
      strip.SetPixelColor(0, black);
      strip.Show();
    }
    if (cycle_counter == 100){
      continue;
    }
    
    unsigned int packetSize = udp.parsePacket();
    if (packetSize != 0) {
      unsigned int len = udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      int nnn = myParsePacket(packetBuffer, len);
      if (nnn == 0){
        String respUDP = "command done!";
        sendPhraseUDP(respUDP);
      }
    }
  } // for
} // loop


/* functions  */

int parseCommand(char packet[], int pCom, int pDat, int len){
  // todo: add service command [sendStatus, setData, getData, setKey, getKey]
  for(int ii = pDat; ii < (len-4); ii++){
    byte pixN = packet[ii]; ii++;
    Serial.print(pixN);
    byte pixR = packet[ii]; ii++;
    Serial.print(pixR);
    byte pixG = packet[ii]; ii++;
    Serial.print(pixG);
    byte pixB = packet[ii];
    Serial.println(pixB);
    RgbColor myColor(pixR,pixG,pixB);
    strip.SetPixelColor(pixN, myColor);
  }
  strip.Show();
  return 0;
}

/*** common WifI UDP Block */
int myParsePacket(char packet[], int packetSize){
  packet[packetSize] = 0;  //stopgag
  unsigned int minLen = unitName.length()+2;
  if (packetSize < minLen){
    return 1; // too short packet
  }
  String strr = String(packet);
  int pCommand = strr.indexOf(".", 0);
  String packetUnitName = strr.substring(0, pCommand);
  if (not packetUnitName.equalsIgnoreCase(unitName)){
    return 2; // packet is not for me
  }
  pCommand++; 
  int pData = strr.indexOf(".", pCommand);
  pData++;
  int res = parseCommand(packet, pCommand, pData, packetSize);
  return res;
}

void sendPhraseUDP(String phrase){
  udp.beginPacket(udpAddress,udpPort);
  udp.print(unitName);
  udp.print(": ");
  udp.print(phrase);
  udp.endPacket();
}

void setupCommon(){
  unitName = String(UNIT_NAME);
  wifiState = DISCONNECTED;  //Connect to the WiFi network
  connectToWiFi(ssid, password);
}

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.disconnect(true);    // delete old config
  wifiState = DISCONNECTED;
  WiFi.onEvent(WiFiEvent); //register event handler
  WiFi.begin(ssid, pwd);  //Initiate connection
  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          wifiState = JUST_CONNECTED;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          wifiState = DISCON_TRIED;
          break;
      default: break;
    }
}
