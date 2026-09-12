#pragma once

#include <Arduino.h>

#if defined(UGV_BOARD_ESP32S3)
const uint8_t PIN_RC_CH1 = 13;
const uint8_t PIN_RC_CH2 = 15;
const uint8_t PIN_RC_CH3 = 14;
const uint8_t PIN_RC_CH5 = 12;

const uint8_t PIN_MOTOR_R_PWM = 5;
const uint8_t PIN_MOTOR_R_REV = 4;
const uint8_t PIN_MOTOR_L_PWM = 2;
const uint8_t PIN_MOTOR_L_REV = 0;

#elif defined(UGV_BOARD_ESP32D)
const uint8_t PIN_RC_CH1 = 13;
const uint8_t PIN_RC_CH2 = 15;
const uint8_t PIN_RC_CH3 = 14;
const uint8_t PIN_RC_CH5 = 12;

const uint8_t PIN_MOTOR_R_PWM = 5;
const uint8_t PIN_MOTOR_R_REV = 33;
const uint8_t PIN_MOTOR_L_PWM = 25;
const uint8_t PIN_MOTOR_L_REV = 34;

#elif defined(UGV_BOARD_PRO_MINI)
const uint8_t PIN_RC_CH1 = A1;
const uint8_t PIN_RC_CH2 = A2;
const uint8_t PIN_RC_CH3 = A3;
const uint8_t PIN_RC_CH5 = A0;

const uint8_t PIN_MOTOR_R_PWM = 5;
const uint8_t PIN_MOTOR_R_REV = 6;
const uint8_t PIN_MOTOR_L_PWM = 10;
const uint8_t PIN_MOTOR_L_REV = 9;

#else
#error "Define one UGV_BOARD_PRO_MINI, UGV_BOARD_ESP32D, or UGV_BOARD_ESP32S3"
#endif