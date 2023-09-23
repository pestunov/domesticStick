/*
 * Base: ESP32
 * module intend for transport data 
 * in: UDP module --> out: I/O ports
 * 
 * received data format
 * "unit#001.12.1"
 * 
 * uses:
 * WiFi ver 1.2.7 by Arduino
 * 
 */

#include <WiFi.h>
#include <WiFiUdp.h>

#include "/home/pi/secure.h"
//#include "D:/1_Projects/secure.h"

#define CYCLE_MAX 200
#define UNIT_NAME "UNIT001"

String unitName, tempStr; 

const char* ssid = SECURE_SSID;          //secure.h #define SECURE_SSID = "my_ssid12345"
const char* password = SECURE_PASSWORD;  //secure.h 
const char* udpAddress = "192.168.001.255"; // a network broadcast address
const int udpPort = 3334;

enum {
  JUST_CONNECTED,
  CONNECTED,
  DISCONNECTED,
  DISCON_TRIED
  } wifiState;

char packetBuffer[1000]; // buffer to hold incoming packet

boolean tag;
int parseCounter;
int cycle_counter = 0;
char my_status, my_tasks, parsePort, parseState;

WiFiUDP udp; // udp library class

int relayPins[] = {0, 4, 17, 2000}; // an array of port to which relay are attached
int statePins[] = {0, 0, 0}; // init state of pins
int pinCount = 3;  


void setup(){
  unitName = String(UNIT_NAME);
  setPins();   // initialize digital pins
  Serial.begin(115200);  // serial port for status monitoring

  wifiState = DISCONNECTED;  //Connect to the WiFi network
  connectToWiFi(ssid, password);

} // setup

void loop()
{
  for(int cycle_counter = 0; cycle_counter < CYCLE_MAX; cycle_counter++){
    delay(10);
    if ((cycle_counter == 10) && (wifiState == JUST_CONNECTED)){ //only send data when connected
      wifiState = CONNECTED;
      tempStr = unitName + " started!";
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.print(tempStr);
      udp.endPacket();
    }
    if ((cycle_counter == 12) && (wifiState == CONNECTED)){ //only send data when connected
      udp.beginPacket(udpAddress,udpPort);
      udp.print(unitName);
      udp.printf(" still here %lu", millis()/1000);
      udp.endPacket();
    }
    if ((cycle_counter == 20) && (wifiState == DISCON_TRIED)){ // try to reconnect
      wifiState = DISCONNECTED;
      connectToWiFi(ssid, password);
    }
    if ((cycle_counter == 30) && (wifiState == DISCONNECTED)){ // still alife
      Serial.print('.');
    }
    if (cycle_counter == 100){
      continue;
    }
    
    unsigned int packetSize = udp.parsePacket();
    if (packetSize) {
      unsigned int len = udp.read(packetBuffer, 255);
      
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      String tStr = String(packetBuffer);
      tStr.trim();
      tStr.toUpperCase();
      Serial.println(tStr);
      int nnn = parseCommand(tStr);
      if (nnn == 0){  // packet acknowleged 
      // send a reply, to the IP address and port that sent us the packet we received
        tempStr = unitName + " done!";
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(tempStr);
        udp.endPacket();
      }
    }
    setPins();
    
  } // for
} // loop

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

int parseCommand(String strr){
  unsigned int len = strr.length();
  unsigned int minLen = unitName.length()+2;
  if (len < minLen){
    return 1;
  } 
  if (not strr.startsWith(unitName)){
    return 2; 
  }
  // todo: add service command [sendStatus, setData, getData, setKey, getKey]
  int firstDot = strr.indexOf(".",unitName.length());
  int secondDot = strr.indexOf(".", firstDot+1);
  String sstr1 = strr.substring(firstDot+1,secondDot);
  String sstr2= strr.substring(secondDot+1);
  unsigned int addr = sstr1.toInt();
  int value = sstr2.toInt();
  Serial.print("Address: ");
  Serial.print(addr);
  Serial.print(", Value: ");
  Serial.println(value);
  if (addr > pinCount){
    return 3;
  }
  statePins[addr] = value;
  return 0;
}

void sendUDP(String strr, const char * udpAddress, int udpPort){
  udp.beginPacket(udpAddress, udpPort);
  udp.print(strr);
  udp.endPacket();
}

/*void sendParsingStatus(int n, String tStr){
  Serial.print("N = ");
  Serial.print(n);
  Serial.print("; String = ");
  Serial.println(tStr);
}
*/
/*
      Serial.print("Received packet of size "); Serial.println(packetSize);
      IPAddress remoteIp = udp.remoteIP();
      Serial.print("From "); Serial.print(remoteIp);
      Serial.print(", port ");  Serial.println(udp.remotePort());
       read the packet into packetBufffer
*/

void setPins(){
  for (int pin = 1; pin < pinCount; pin++) {
    if (relayPins[pin] >= 1000){
      break;
    }
    digitalWrite(relayPins[pin], (statePins[pin]==1)? HIGH : LOW);   // turn the port off
    pinMode(relayPins[pin], OUTPUT);     // pin as output
  }
}
