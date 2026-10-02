#pragma once

#ifndef DEBUG_MODE
  #define DEBUG_MODE 0
#endif

#if DEBUG_MODE == 0
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(value)
  #define DEBUG_PRINTLN(value)
#else
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
  #define DEBUG_PRINT(value) Serial.print(value)
  #define DEBUG_PRINTLN(value) Serial.println(value)

  //++ FS LOGS
#endif