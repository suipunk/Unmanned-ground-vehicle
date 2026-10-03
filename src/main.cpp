#include <Arduino.h>
#include "debugPrint.h"
#include "Pins.h"
#include "ctrlFunctions.h"
#include "RadioConfig.h"
#include "radioCtrl.h"
#include "telemetryLog.h"




int currentLeft = 0;
int currentRight = 0;
bool reverseMode = false;
bool safetyStop = true;
unsigned long lastDebugMs = 0;
unsigned long ch5DebounceTimer = 0;
int lastCh5Value = 0;
int stableCh5Value = 0;
const unsigned long DEBOUNCE_DELAY_MS = 50;




int centerChannelToSigned(int pulse, int reverse) {
  int value = (pulse - RC_MID_US) * reverse;

  if (abs(value) <= STICK_DEADBAND_US) {
    return 0;
  }

  if (value > 0) {
    return map(value, STICK_DEADBAND_US, RC_MAX_US - RC_MID_US, 0, MOTOR_PWM_MAX);
  }

  return map(value, -STICK_DEADBAND_US, RC_MIN_US - RC_MID_US, 0, -MOTOR_PWM_MAX);
}

int throttleToPower(int pulse) {
  if (CH3_REVERSE == -1) {
    pulse = RC_MAX_US - (pulse - RC_MIN_US);
  }

  pulse = constrain(pulse, RC_MIN_US, RC_MAX_US);

  if (pulse <= THROTTLE_START_US) {
    return 0;
  }

  return map(pulse, THROTTLE_START_US, RC_MAX_US, 0, MOTOR_PWM_MAX);
}

int rampTo(int current, int target) {
  int step = abs(target) > abs(current) ? RAMP_STEP_UP : RAMP_STEP_DOWN;

  if (current < target) {
    return min(current + step, target);
  }

  if (current > target) {
    return max(current - step, target);
  }

  return current;
}

void writeMotor(uint8_t pwmPin, uint8_t revPin, int speedValue) {
  bool reverse = speedValue < 0;
  speedValue = constrain(abs(speedValue), 0, MOTOR_PWM_MAX);

  digitalWrite(revPin, reverse ? HIGH : LOW);
  analogWrite(pwmPin, speedValue);
}

void writeMotors(int leftTarget, int rightTarget) {
  currentLeft = rampTo(currentLeft, leftTarget);
  currentRight = rampTo(currentRight, rightTarget);

  writeMotor(PIN_MOTOR_L_PWM, PIN_MOTOR_L_REV, currentLeft);
  writeMotor(PIN_MOTOR_R_PWM, PIN_MOTOR_R_REV, currentRight);
}

void stopMotorsNow() {
  DEBUG_PRINTLN("Stopping motors immediately");
  currentLeft = 0;
  currentRight = 0;
  analogWrite(PIN_MOTOR_L_PWM, 0);
  analogWrite(PIN_MOTOR_R_PWM, 0);
  digitalWrite(PIN_MOTOR_L_REV, LOW);
  digitalWrite(PIN_MOTOR_R_REV, LOW);
  DEBUG_PRINTLN("Motors stopped");
}

// Helpers shared between the debounce step and the safety-stop branch
bool isThrottleLow(const RcInput &rc) {
  return rc.ch3 <= REVERSE_ARM_MAX_US;
}

bool isReverseRequested(const RcInput &rc) {
  return rc.ch5Ok && stableCh5Value < CH5_REVERSE_THRESHOLD_US;
}



//loopsplit into functions
// Returns true if the caller should stop processing this cycle (invalid RC signal)
bool handleInvalidSignal(const RcInput &rc) {
  if (rc.valid) {
    return false;
  }

  safetyStop = true;
  stopMotorsNow();

  if (millis() - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = millis();
    DEBUG_PRINT("INVALID SIGNAL ch1=");
    DEBUG_PRINT(rc.ch1);
    DEBUG_PRINT(" raw1=");
    DEBUG_PRINT(rc.ch1Raw);
    DEBUG_PRINT(" ok1=");
    DEBUG_PRINT(rc.ch1Ok);
    DEBUG_PRINT(" ch3=");
    DEBUG_PRINT(rc.ch3);
    DEBUG_PRINT(" raw3=");
    DEBUG_PRINT(rc.ch3Raw);
    DEBUG_PRINT(" ok3=");
    DEBUG_PRINT(rc.ch3Ok);
    DEBUG_PRINT(" ch5=");
    DEBUG_PRINT(rc.ch5);
    DEBUG_PRINT(" raw5=");
    DEBUG_PRINT(rc.ch5Raw);
    DEBUG_PRINT(" ok5=");
    DEBUG_PRINT(rc.ch5Ok);
    DEBUG_PRINT(" step=");
    DEBUG_PRINTLN(rc.readStep);
  }

  logTelemetryRow("INVALID", rc, 0, 0, 0, 0, currentLeft, currentRight,
                   reverseMode, false, false);
  return true;
}

// Debounces ch5 and updates reverseMode and safetyStop 
void updateReverseDebounce(const RcInput &rc) {
  // Debounce ch5 input to avoid rapid toggling of reverse mode
  if (abs(rc.ch5 - lastCh5Value) > 20) {
    ch5DebounceTimer = millis();
    lastCh5Value = rc.ch5;
  }

  // if signal didnt change more than 50 ms
  if ((millis() - ch5DebounceTimer) > DEBOUNCE_DELAY_MS) {
    if (stableCh5Value == 0) stableCh5Value = rc.ch5;
    stableCh5Value = rc.ch5;
  }

  bool throttleLow = isThrottleLow(rc);
  bool requestedReverse = isReverseRequested(rc);

  if (requestedReverse != reverseMode) {
    if (throttleLow) {
      reverseMode = requestedReverse;
      safetyStop = false;
    } else {
      safetyStop = true;
    }
  }

  if (throttleLow) {
    safetyStop = false;
  }
}

// Returns true if the caller should stop processing this cycle (safety stop active)
bool handleSafetyStop(const RcInput &rc) {
  if (!safetyStop) {
    return false;
  }

  writeMotors(0, 0);

  bool throttleLow = isThrottleLow(rc);
  bool requestedReverse = isReverseRequested(rc);

  if (millis() - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = millis();
    DEBUG_PRINT("SAFETY STOP ch3=");
    DEBUG_PRINT(rc.ch3);
    DEBUG_PRINT(" ch5=");
    DEBUG_PRINT(rc.ch5);
    DEBUG_PRINT(" reverse=");
    DEBUG_PRINT(reverseMode);
    DEBUG_PRINT(" throttleLow=");
    DEBUG_PRINT(throttleLow);
    DEBUG_PRINT(" requestedReverse=");
    DEBUG_PRINTLN(requestedReverse);
  }

  logTelemetryRow("SAFETY", rc, 0, 0, 0, 0, currentLeft, currentRight,
                   reverseMode, throttleLow, requestedReverse);
  return true;
}

// Computes targets, drives motors, logs/debugs
void driveMotors(const RcInput &rc) {
  int power = throttleToPower(rc.ch3);
  int ch1Turn = centerChannelToSigned(rc.ch1, CH1_REVERSE);
  int steering = constrain(-ch1Turn, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);

  int leftTarget = constrain(power + steering, 0, MOTOR_PWM_MAX);
  int rightTarget = constrain(power - steering, 0, MOTOR_PWM_MAX);

  if (reverseMode) {
    leftTarget = -leftTarget;
    rightTarget = -rightTarget;
  }

  writeMotors(leftTarget, rightTarget);

  if (millis() - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = millis();
    DEBUG_PRINT("ch1=");
    DEBUG_PRINT(rc.ch1);
    DEBUG_PRINT(" raw1=");
    DEBUG_PRINT(rc.ch1Raw);
    DEBUG_PRINT(" ch3=");
    DEBUG_PRINT(rc.ch3);
    DEBUG_PRINT(" raw3=");
    DEBUG_PRINT(rc.ch3Raw);
    DEBUG_PRINT(" ch5=");
    DEBUG_PRINT(rc.ch5);
    DEBUG_PRINT(" raw5=");
    DEBUG_PRINT(rc.ch5Raw);
    DEBUG_PRINT(" ok1=");
    DEBUG_PRINT(rc.ch1Ok);
    DEBUG_PRINT(" ok3=");
    DEBUG_PRINT(rc.ch3Ok);
    DEBUG_PRINT(" ok5=");
    DEBUG_PRINT(rc.ch5Ok);
    DEBUG_PRINT(" step=");
    DEBUG_PRINT(rc.readStep);
    DEBUG_PRINT(" power=");
    DEBUG_PRINT(power);
    DEBUG_PRINT(" steering=");
    DEBUG_PRINT(steering);
    DEBUG_PRINT(" targetL=");
    DEBUG_PRINT(leftTarget);
    DEBUG_PRINT(" targetR=");
    DEBUG_PRINT(rightTarget);
    DEBUG_PRINT(" currentL=");
    DEBUG_PRINT(currentLeft);
    DEBUG_PRINT(" currentR=");
    DEBUG_PRINT(currentRight);
    DEBUG_PRINT(" reverse=");
    DEBUG_PRINT(reverseMode);
    DEBUG_PRINT(" revL=");
    DEBUG_PRINT(currentLeft < 0);
    DEBUG_PRINT(" revR=");
    DEBUG_PRINTLN(currentRight < 0);
  }

  logTelemetryRow("DRIVE", rc, power, steering, leftTarget, rightTarget,
                   currentLeft, currentRight, reverseMode, false, false);
}



void setup() {
  setupDebugMode();
  setupInputPins();
  setupOutputPins();
  stopMotorsNow();  
  setupTelemetryLog();
  DEBUG_PRINTLN("Setup ended");
}

void loop() {
  RcInput rc = readRadio();

  if (!validateRadioSignal(rc)) {
    handleInvalidSignal(rc);
    return;
  }

  updateReverseDebounce(rc);

  if (handleSafetyStop(rc)) {
    return;
  }

  driveMotors(rc);
}