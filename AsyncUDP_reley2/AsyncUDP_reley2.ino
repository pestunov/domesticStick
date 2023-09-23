#include <WiFi.h>
#include <AsyncUDP.h>

#include "secure.h"
#include "cipher.h"

#define RELEY_ON LOW
#define RELEY_OFF HIGH

#define RELEY_1_PIN 16
#define RELEY_2_PIN 17
#define RELEY_3_PIN 5
#define RELEY_4_PIN 18

uint8_t releyPins[] = {RELEY_1_PIN, RELEY_2_PIN, RELEY_3_PIN, RELEY_4_PIN, 0};
uint8_t reley_stat = 0x00;  // all pins are disactivate

const char *ssid = SECURE_SSID;
const char *password = SECURE_PASSWORD;
const char *module_name = "reley_module_00001";

String udpAddress;
uint16_t udpPort = 33333;
uint16_t myUdpPort = 50010;

//Are we currently connected?
boolean connected = false;
//The udp library class
AsyncUDP udp;


void connectToWiFi(const char* ssid, const char* pwd) {
  // delete old config
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  //Initiate connection
  WiFi.begin(ssid, pwd);
  Serial.println("Connecting to WiFi network: " + String(ssid));
}

// Определяем callback функцию обработки пакета
void parsePacket(AsyncUDPPacket packet) {
  // Записываем адрес начала данных в памяти
  const uint8_t* msg = packet.data();
  // Записываем размер данных
  const size_t len = packet.length();
  // Если адрес данных не равен нулю и размер данных больше нуля...
  Serial.print("Got packet: ");
  if (msg != NULL && len >= 4) {
    for (uint8_t i = 0; i < len; i++) {
      if (msg[i] == 0x31) {
        reley_stat |= (1 << i);
        Serial.printf(" relay %d is on!\n", i);
      } else {
        reley_stat &= ~(1 << i);
        Serial.printf(" relay %d is off!\n", i);
      }
    }
  }
  set_reley_pins();
}

void set_reley_pins() {
  for (uint8_t i = 0; i < 100; i++) {
    if (releyPins[i] == 0) break;
    if ((reley_stat & (1 << i)) != 0) digitalWrite(releyPins[i], RELEY_ON); else digitalWrite(releyPins[i], RELEY_OFF);
  }
}

void setup() {
  // Initilize hardware serial:
  Serial.begin(115200);
  delay(100);

  //Connect to the WiFi network
  connectToWiFi(ssid, password);
  delay(100);

  set_reley_pins();
  pinMode(RELEY_1_PIN, OUTPUT);
  pinMode(RELEY_2_PIN, OUTPUT);
  pinMode(RELEY_3_PIN, OUTPUT);
  pinMode(RELEY_4_PIN, OUTPUT);
}

void loop() {
  set_reley_pins();
  switch(WiFi.status()) {
    case WL_NO_SSID_AVAIL:
      connected = false;
      Serial.println("[WiFi] SSID not found");
      break;
    case WL_CONNECT_FAILED:
      connected = false;
      Serial.println("[WiFi] Failed - WiFi not connected!");
      break;
    case WL_CONNECTION_LOST:
      connected = false;
      Serial.println("[WiFi] Connection was lost");
      break;
    case WL_DISCONNECTED:
      connected = false;
      Serial.println("[WiFi] WiFi is disconnected");
      break;
    case WL_CONNECTED:
      if (!connected) {
        connected = true;
        udpAddress = String(WiFi.localIP()[0]) + "." + 
                    String(WiFi.localIP()[1]) + "." + 
                    String(WiFi.localIP()[2]) + ".255";
        Serial.print("[WiFi] WiFi is connected! Got IP address: ");
        Serial.print(WiFi.localIP());
        Serial.print("; broadcast address: ");
        Serial.println(udpAddress);
        if(udp.listen(myUdpPort)) {
          // При получении пакета вызываем callback функцию
          udp.onPacket(parsePacket);
        }

      }
      break;
    default:
      Serial.print("[WiFi] WiFi Status: ");
      Serial.println(WiFi.status());
      break;
  }  // switch
  if (connected) {
    //Send a packet
    String toSend = "hello server; I_am: ";
    toSend += module_name;
    toSend += "; timer: ";
    toSend += millis();
    toSend += "; my_port: ";
    toSend += myUdpPort;
    toSend += "; reley_status: ";
    toSend += reley_stat;

    udp.broadcastTo(toSend.c_str(), udpPort);
    //udp.printf("timer: %lu; ", millis() / 1000);
    //udp.printf("reley status: %s; ", String(reley_stat, BIN).c_str());

    Serial.printf("hereis: %s; ", module_name);
    Serial.printf("Seconds since boot: %lu; ", millis() / 1000);
    Serial.printf("reley status: %d; ", reley_stat);
    Serial.println("");
  
  }
  if (!connected) {
    //Connect to the WiFi network
    connectToWiFi(ssid, password);
    delay(5000);
  }
  delay(4999);

}
