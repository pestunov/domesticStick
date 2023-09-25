#include <WiFi.h>
#include <AsyncUDP.h>

#include "secure.h"
#include "cipher.h"

#define DEBUG

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

uint16_t udpPort = 33333;
uint16_t myUdpPort = 50010;

//Are we currently connected?
boolean connected = false;
//The udp library class
AsyncUDP udp;

// main cycle counter
uint8_t mainCycleCounter = 0;

void serialPrint(String inStr) {
#ifdef DEBUG
  Serial.print(inStr);
#endif
}

void serialPrintln(String inStr) {
#ifdef DEBUG
  Serial.println(inStr);
#endif
}

void connectToWiFi(const char* ssid, const char* pwd) {
  // delete old config
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  //Initiate connection
  WiFi.begin(ssid, pwd);
  serialPrintln("Connecting to WiFi network: " + String(ssid));
}

boolean isWiFiConnected() {
  switch(WiFi.status()) {
    case WL_NO_SSID_AVAIL:
      serialPrintln("[WiFi] SSID not found");
      return false;
    case WL_CONNECT_FAILED:
      serialPrintln("[WiFi] Failed - WiFi not connected!");
      return false;
    case WL_CONNECTION_LOST:
      serialPrintln("[WiFi] Connection was lost");
      return false;
    case WL_DISCONNECTED:
      serialPrintln("[WiFi] WiFi is disconnected");
      return false;
    case WL_CONNECTED:
      serialPrintln("[WiFi] WiFi is connected!");
      return true;
    default:
      serialPrintln("[WiFi] WiFi Status: " + WiFi.status());
      return false;
  }  // switch
  return false;
}

String getUDPAddress() {
  String res = String(WiFi.localIP()[0]) + "." +
               String(WiFi.localIP()[1]) + "." + 
               String(WiFi.localIP()[2]) + ".255";
  return res;
}

String getStatusString() {
  String res = "hello server; I_am: ";
  res += module_name;
  res += "; timer: ";
  res += millis();
  res += "; my_port: ";
  res += myUdpPort;
  res += "; relay_status: ";
  res += relay_stat;
  res += "; button_status: ";
  res += button_stat;
  return res;
}

// Определяем callback функцию обработки пакета
void parsePacket(AsyncUDPPacket packet) {
  // Записываем адрес начала данных в памяти
  const uint8_t* msg = packet.data();
  // Записываем размер данных
  const size_t len = packet.length();
  // Если адрес данных не равен нулю и размер данных больше нуля...
  serialPrint("Got packet: ");
  if (msg != NULL && len >= 4) {
    for (uint8_t i = 0; i < len; i++) {
      if (msg[i] == 0x31) {
        relay_stat |= (1 << i);
        serialPrint(" relay " + String(i) + " is on!");
      } else {
        relay_stat &= ~(1 << i);
        serialPrint(" relay " + String(i) + " is off!");
      }
    }
  }
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
  mainCycleCounter++;
  if (mainCycleCounter == 2) {
    if (isWiFiConnected()) {
      if (!connected) {
        connected = true;
        serialPrintln("[WiFi] WiFi is connected! Got IP address: " + String(WiFi.localIP()));
        serialPrintln("broadcast address: " + getUDPAddress());
        if(udp.listen(myUdpPort)) {  // При получении пакета вызываем callback функцию
          udp.onPacket(parsePacket);
        }
      }
      String toSend = getStatusString();
      udp.broadcastTo(toSend.c_str(), udpPort);
      serialPrintln(toSend);
    } else {  
      //not connected to the WiFi network
      connected = false;
      connectToWiFi(ssid, password);
    }

  }
  // if (mainCycleCounter == 3) { // do some cyclique}

  get_button_state();
  set_relay_pins();
  delay(10);
}  // loop
