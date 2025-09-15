// debug_port.h
#pragma once

#ifdef WAVSHARE
  #define DEBUG_PORT Serial
#else
  #define DEBUG_PORT Serial0
#endif

