// debug_port.h
#pragma once
#include <Arduino.h>       // Ensures Serial0 is declared
#include <HardwareSerial.h>
extern HardwareSerial Serial0;  // UART0
//cdc on boot should be enabled for Serial0?  ??ver 2.0.17 esp does not have Serial0 needs Serial2 ???
 #ifdef WAVSHARE
   #define DEBUG_PORT Serial
 #else
  #define DEBUG_PORT Serial //was Serial0 before I started changing the USB CDC on boot command manually ! But if set her, leads to issues with the Arduino CLI if used.
#endif

