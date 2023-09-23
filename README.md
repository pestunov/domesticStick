# domesticStick  
STM32 & AVR & ESP32 modules for smartdacha units   
## AsyncUDPClient_Relay2  

renewed project `AsyncUDPClient_RelayUnit`  
Several (four) power switches/relays controller with associated buttons  
>  to do:
>  - [ ] commands to on/off each power switch
>  - [ ] buttons for manual switches control

## AsyncUDPServer_Neopixel module  

ESP32 platform. this module is a bridge between the local network and ws2812 bus
 
1. Connect to WiFi AP/router using your ssid and password
2. Indicate status by 1st ic ws2812
2. Initialaze UDP listen procedure
  1. get packet
  2. slide thru packet, pushing bytes into neoPixel massive

```c
        while (ii < len) {
          byte pixN = packet.data()[ii]; ii++;
          byte pixR = packet.data()[ii]; ii++;
          byte pixG = packet.data()[ii]; ii++;
          byte pixB = packet.data()[ii]; ii++;
          RgbColor myColor(pixR,pixG,pixB);
          strip.SetPixelColor(pixN, myColor);
        }
        strip.Show();
```
        
3. Send broadcast **hey** each 2 seconds

> ### received data format *version 0.1*
> 
> (x*4)th byte - pixel num  
> (x*4)+1th byte - x pixel red brightness  
> (x*4)+2th byte - x pixel green brightness  
> (x*4)+3th byte - x pixel blue brightness  

###### to do
- [ ] debug wifi init. probability of successful launch is 50%  
- [ ] add the ability to address modules  
