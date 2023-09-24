#include <WiFi.h>
#include <AsyncUDP.h>

#include "secure.h"
#include "cipher.h"

#define RELAY_ON LOW
#define RELAY_OFF HIGH

// buttons state in case of NO pin used, with PU resistor
#define BUTTON_PRESSED LOW
#define BUTTON_UNPRESSED HIGH

#define RELAY_1_PIN 16
#define RELAY_2_PIN 17
#define RELAY_3_PIN 18
#define RELAY_4_PIN 19

#define BUTTON_1_PIN 0    // 0
#define BUTTON_2_PIN 5    // 5
#define BUTTON_3_PIN 14    // 14
#define BUTTON_4_PIN 15    // 15

uint8_t relayPins[] = {RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN, 0xff};
uint8_t buttonPins[] = {BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN, BUTTON_4_PIN, 0xff};

uint8_t relay_stat = 0x00;  // all pins are disactivate
uint8_t button_stat = 0x00;  // all buttons unpressed
uint8_t button_prev = button_stat;  // make toggle 'button_stat' after push button

const char *ssid = SECURE_SSID;
const char *password = SECURE_PASSWORD;
const char *module_name = "relay_module_00001";
const char *module_id = "kalyabalya";

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
        relay_stat |= (1 << i);
        Serial.printf(" relay %d is on!\n", i);
      } else {
        relay_stat &= ~(1 << i);
        Serial.printf(" relay %d is off!\n", i);
      }
    }
  }
  set_relay_pins();
}

void set_relay_pins() {
  for (uint8_t i = 0; i < 8; i++) {
    if (relayPins[i] == 0xff) break;
    if ((relay_stat & (1 << i)) != 0) digitalWrite(relayPins[i], RELAY_ON); else digitalWrite(relayPins[i], RELAY_OFF);
  }
}

boolean get_button_state() {
  boolean res = false;  // return true if `button_stat` changed
  for (uint8_t i = 0; i < 8; i++) {
    if (buttonPins[i] == 0xff) break;
    if (digitalRead(buttonPins[i]) == BUTTON_PRESSED) {
      if ( (button_stat & (1 << i)) == (button_prev & (1 << i)) ) {
        button_stat ^= (1 << i);
        res = true;
      }
    } else {
      (button_stat & (1 << i)) ? button_prev |= (1 << i) : button_prev &= ~(1 << i);
    }
  }
  return res;
}

void setup() {
  // Initilize hardware serial:
  Serial.begin(115200);
  delay(100);

  //Connect to the WiFi network
  connectToWiFi(ssid, password);
  delay(100);

  set_relay_pins();
  delay(100);

  //  setting up the input pins
  for (uint8_t i = 0; i < 8; i++) {
    if (buttonPins[i] == 0xff) break;
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  //  setting up the output pins
  for (uint8_t i = 0; i < 8; i++) {
    if (relayPins[i] == 0xff) break;
    pinMode(relayPins[i], OUTPUT);
  }
}

void loop() {
  set_relay_pins();
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
    toSend += "; relay_status: ";
    toSend += relay_stat;
    toSend += "; button_status: ";
    toSend += button_stat;

    udp.broadcastTo(toSend.c_str(), udpPort);
    Serial.println(toSend);
  } else {
    //Connect to the WiFi network
    connectToWiFi(ssid, password);
    delay(5000);
  }

  for (uint16_t i = 0; i < 200; i++){
    get_button_state();
    delay(10);
  }

}  // loop
