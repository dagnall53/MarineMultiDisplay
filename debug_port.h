// debug_port.h
#pragma once
//cdc on boot should be enabled for Serial0?  ??ver 2.0.17 esp does not have Serial0 needs Serial2 ???
#ifdef WAVSHARE
  #define DEBUG_PORT Serial
#else
  #define DEBUG_PORT Serial0 //
#endif

