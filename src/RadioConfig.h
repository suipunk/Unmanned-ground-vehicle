#pragma once

#include <Arduino.h>

const int RC_MIN_US = 1000;
const int RC_MID_US = 1500;
const int RC_MAX_US = 2000;
const int RC_VALID_MIN_US = 900;
const int RC_VALID_MAX_US = 2100;
const unsigned long RC_TIMEOUT_US = 25000;
const unsigned long SIGNAL_LOSS_TIMEOUT_MS = 300;

const int STICK_DEADBAND_US = 35;
const int THROTTLE_START_US = 1080;
const int REVERSE_ARM_MAX_US = 1120;

const int CH5_REVERSE_THRESHOLD_US = 1300;

const int MOTOR_PWM_MAX = 255;

// #define DEBUG_MODE 1
const unsigned long DEBUG_INTERVAL_MS = 200;
const unsigned long SERIAL_BAUD = 9600;

#ifdef DEBUG_MODE
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
  #define DEBUG_PRINT(value) Serial.print(value)
  #define DEBUG_PRINTLN(value) Serial.println(value)
#else
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(value)
  #define DEBUG_PRINTLN(value)
#endif

const int RAMP_STEP_UP = 4;
const int RAMP_STEP_DOWN = 10;

const int CH1_REVERSE = 1;
const int CH3_REVERSE = -1;
