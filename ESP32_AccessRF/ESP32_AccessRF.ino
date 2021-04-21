/*
 * Base: ESP32
 * dome access controller
 * uses: rfid RC522
 * 
    RC-522 | Arduino| ESP32
    -------------------------
   SDA(SS) | D      | 21
       SCK | D      | 18
      MOSI | D      | 23
      MISO | D      | 19
       IRQ | N/A    | N/A
       GND | GND    | GND
       RST | D      | 22
      3.3V | 3.3V   | 3.3V
      
 * WiFi ver 1.2.7 by Arduino
 */
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

/*** start common WiFi UDP block */
#include <WiFi.h>
#include <WiFiUdp.h>
#include "/home/pi/secure.h"
//#include "D:/1_Projects/secure.h"

#define CYCLE_PERIOD 200
#define UNIT_NAME "UNIT_RF001"
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

/* rf id setup */
uint8_t successRead;
byte readCard[4];

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
/* end rf id setup */

void setup(){
  Serial.begin(115200);
  setupCommon();
  SPI.begin();                      // Init SPI bus
  mfrc522.PCD_Init();               // Init MFRC522 card
  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

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
//      strip.SetPixelColor(0, green);
//      strip.Show();

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
//      strip.SetPixelColor(0, red);
//      strip.Show();
    }
    if ((cycle_counter == 16) && (wifiState == DISCONNECTED)){ // still disconnected
      Serial.print('.');
      tag = true;
//      strip.SetPixelColor(0, pale_red);
//      strip.Show();
    }
    if ((cycle_counter == 20) && (tag)){
      tag = false;
//      strip.SetPixelColor(0, black);
//      strip.Show();
    }

    if ((cycle_counter % 10) == 0){
      if (getRFID() == 0) {
        udp.beginPacket(udpAddress,udpPort);
        udp.print(unitName);
        udp.print(": ");
        for ( uint8_t i = 0; i < 4; i++) {  //
          udp.print(mfrc522.uid.uidByte[i]);
        }
        udp.endPacket();
      }
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
  // packet = received bytes, pCom = pointer to command, pDat = pointer to data, len = pocket lenght
  // todo: add service command [sendStatus, setData, getData, setKey, getKey]
  return 0; // 0 - all ok
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

uint8_t getRFID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 1;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 1;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], DEC);
    Serial.print("*");
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 0;
}
