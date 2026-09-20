#include <Arduino.h>
#include "debugPrint.h"
#include "Pins.h"
#include "RadioConfig.h"


void setupDebugMode()
{
  DEBUG_BEGIN(SERIAL_BAUD);
  delay(10);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Setup begin");
  DEBUG_PRINT("Pins RC ch1=");
  DEBUG_PRINT(PIN_RC_CH1);
  DEBUG_PRINT(" ch3=");
  DEBUG_PRINT(PIN_RC_CH3);
  DEBUG_PRINT(" ch5=");
  DEBUG_PRINTLN(PIN_RC_CH5);
  DEBUG_PRINT("Pins motor R pwm=");
  DEBUG_PRINT(PIN_MOTOR_R_PWM);
  DEBUG_PRINT(" rev=");
  DEBUG_PRINT(PIN_MOTOR_R_REV);
  DEBUG_PRINT(" L pwm=");
  DEBUG_PRINT(PIN_MOTOR_L_PWM);
  DEBUG_PRINT(" rev=");
  DEBUG_PRINTLN(PIN_MOTOR_L_REV);
  DEBUG_PRINT("Config throttleStart=");
  DEBUG_PRINT(THROTTLE_START_US);
  DEBUG_PRINT(" reverseArm=");
  DEBUG_PRINT(REVERSE_ARM_MAX_US);
  DEBUG_PRINT(" ch5Threshold=");
  DEBUG_PRINT(CH5_REVERSE_THRESHOLD_US);
  DEBUG_PRINT(" rcTimeout=");
  DEBUG_PRINT(RC_TIMEOUT_US);
  DEBUG_PRINT(" signalLoss=");
  DEBUG_PRINTLN(SIGNAL_LOSS_TIMEOUT_MS);
}

void setupInputPins()
{
  DEBUG_PRINTLN("Setup input pins start");
  pinMode(PIN_RC_CH1, INPUT);
  pinMode(PIN_RC_CH2, INPUT);

  pinMode(PIN_RC_CH3, INPUT);
  pinMode(PIN_RC_CH5, INPUT);
  DEBUG_PRINTLN("Setup input pins end");
}

void setupOutputPins()
{
  DEBUG_PRINTLN("Setup output pins start");
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PIN_MOTOR_L_PWM, OUTPUT);
  pinMode(PIN_MOTOR_L_REV, OUTPUT);
  pinMode(PIN_MOTOR_R_PWM, OUTPUT);
  pinMode(PIN_MOTOR_R_REV, OUTPUT);
  DEBUG_PRINTLN("Setup output pins end");
}
