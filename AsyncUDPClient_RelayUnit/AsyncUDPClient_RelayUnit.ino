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

#define RELAY_1 0
#define CYCLE_MAX 200
#define UNIT_NAME "UNIT#001"


// WiFi network name and password:
const char * ssid = SECURE_SSID;          //secure.h #define SECURE_SSID = "my_ssid12345"
const char * password = SECURE_PASSWORD;  //secure.h 

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
const char * udpAddress = "192.168.1.255";
const int udpPort = 3334;
//Are we currently connected?
boolean connected = false;

enum {
  CONNECTED,
  DISCONNECTED,
  DISCON_TRIED
  } wifiState;

char packetBuffer[255];               //buffer to hold incoming packet
char ReplyBuffer[] = "acknowledged"; // a string to send back

boolean tag;
int parseCounter;
int cycle_counter = 0;

char my_status, my_tasks, parsePort, parseState;

// an array of port to which relay are attached
int relayPins[] = {0, 4, 17, 2000};
// init state of pins
int statePins[] = {0, 0, 0};
int pinCount = 3;  

//The udp library class
WiFiUDP udp;


void setup(){
  // initialize digital pins
  setPins();
  // serial port for status monitoring
  Serial.begin(115200);
  
  //Connect to the WiFi network
  wifiState = DISCONNECTED;
  connectToWiFi(ssid, password);

} // setup

void loop()
{
  for(int cycle_counter = 0; cycle_counter < CYCLE_MAX; cycle_counter++){
    delay(10);
    if ((cycle_counter == 0) && (wifiState == CONNECTED)){ //only send data when connected
      udp.beginPacket(udpAddress,udpPort);
      udp.printf("Seconds since boot: %lu", millis()/1000);
      udp.endPacket();
    }
    if ((cycle_counter == 0) && (wifiState == DISCON_TRIED)){ // try to reconnect
      connectToWiFi(ssid, password);
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
      // send a reply, to the IP address and port that sent us the packet we received
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      tStr = UNIT_NAME;
      tStr += " acknownleged at ";
      tStr += WiFi.localIP();
      udp.printf("hello");  
      udp.endPacket();
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
          wifiState = CONNECTED;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          wifiState = DISCON_TRIED;
          break;
      default: break;
    }
}

void setPins(){
  for (int pin = 1; pin < pinCount; pin++) {
    if (relayPins[pin] >= 1000){
      break;
    }
    digitalWrite(relayPins[pin], (statePins[pin]==1)? HIGH : LOW);   // turn the port off
    pinMode(relayPins[pin], OUTPUT);     // pin as output
  }
}

int parseCommand(String strr){
  unsigned int len = strr.length();
  if (len<10){
    return 0;
  } 
  if (not strr.startsWith(UNIT_NAME)){
    return 0; 
  }
  String sName = String(UNIT_NAME);
  
  int firstDot = strr.indexOf(".",sName.length());
  int secondDot = strr.indexOf(".", firstDot+1);
  String sstr1 = strr.substring(firstDot+1,secondDot);
  String sstr2= strr.substring(secondDot+1);
  unsigned int addr = sstr1.toInt();
  int value = sstr2.toInt();
  Serial.println(addr);
  Serial.println(value);
  if (addr > pinCount){
    return 0;
  }
  statePins[addr] = value;
  return 1;
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
