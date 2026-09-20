#include "Pins.h"
#include "RadioConfig.h"

struct RcInput {
  int ch1;
  int ch3;
  int ch5;
  unsigned long ch1Raw;
  unsigned long ch3Raw;
  unsigned long ch5Raw;
  bool ch1Ok;
  bool ch3Ok;
  bool ch5Ok;
  uint8_t readStep;
  bool valid;
};

struct RcChannelState {
  int value;
  unsigned long raw;
  unsigned long lastOkMs;
  bool ok;
};

int currentLeft = 0;
int currentRight = 0;
bool reverseMode = false;
bool safetyStop = true;
unsigned long lastDebugMs = 0;
unsigned long ch5DebounceTimer = 0;
int lastCh5Value = 0;
int stableCh5Value = 0;
const unsigned long DEBOUNCE_DELAY_MS = 50;
RcChannelState ch1State = {0, 0, 0, false}; 
RcChannelState ch3State = {0, 0, 0, false};
RcChannelState ch5State = {0, 0, 0, false}; 
uint8_t rcReadStep = 0;

void updateRcChannel(uint8_t pin, RcChannelState *channel) {
  unsigned long pulse = pulseIn(pin, HIGH, RC_TIMEOUT_US);
  channel->raw = pulse;

  if (pulse >= RC_VALID_MIN_US && pulse <= RC_VALID_MAX_US) {
    channel->value = constrain((int)pulse, RC_MIN_US, RC_MAX_US);
    channel->lastOkMs = millis();
  }

  channel->ok = millis() - channel->lastOkMs <= SIGNAL_LOSS_TIMEOUT_MS;
}

RcInput readRadio() {
  uint8_t currentReadStep = rcReadStep;

  if (currentReadStep == 0) {
    updateRcChannel(PIN_RC_CH1, &ch1State);
  } else if (currentReadStep == 1) {
    updateRcChannel(PIN_RC_CH3, &ch3State);
  } else {
    updateRcChannel(PIN_RC_CH5, &ch5State);
  }

  rcReadStep++;
  if (rcReadStep > 2) {
    rcReadStep = 0;
  }

  unsigned long now = millis();
  ch1State.ok = now - ch1State.lastOkMs <= SIGNAL_LOSS_TIMEOUT_MS;
  ch3State.ok = now - ch3State.lastOkMs <= SIGNAL_LOSS_TIMEOUT_MS;
  ch5State.ok = now - ch5State.lastOkMs <= SIGNAL_LOSS_TIMEOUT_MS;

  RcInput rc;
  rc.ch1 = ch1State.value;
  rc.ch3 = ch3State.value;
  rc.ch5 = ch5State.value;
  rc.ch1Raw = ch1State.raw;
  rc.ch3Raw = ch3State.raw;
  rc.ch5Raw = ch5State.raw;
  rc.ch1Ok = ch1State.ok;
  rc.ch3Ok = ch3State.ok;
  rc.ch5Ok = ch5State.ok;
  rc.readStep = currentReadStep;
  rc.valid = rc.ch1Ok && rc.ch3Ok && rc.ch5Ok;
  return rc;
}

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
  currentLeft = 0;
  currentRight = 0;
  analogWrite(PIN_MOTOR_L_PWM, 0);
  analogWrite(PIN_MOTOR_R_PWM, 0);
  digitalWrite(PIN_MOTOR_L_REV, LOW);
  digitalWrite(PIN_MOTOR_R_REV, LOW);
}

void setup() {
  DEBUG_BEGIN(SERIAL_BAUD);
  delay(10);
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

  DEBUG_PRINTLN("Setup input pins start");
  pinMode(PIN_RC_CH1, INPUT);
    pinMode(PIN_RC_CH2, INPUT);

  pinMode(PIN_RC_CH3, INPUT);
  pinMode(PIN_RC_CH5, INPUT);
  DEBUG_PRINTLN("Setup input pins end");

  DEBUG_PRINTLN("Setup output pins start");
  pinMode(PIN_MOTOR_L_PWM, OUTPUT);
  pinMode(PIN_MOTOR_L_REV, OUTPUT);
  pinMode(PIN_MOTOR_R_PWM, OUTPUT);
  pinMode(PIN_MOTOR_R_REV, OUTPUT);
  DEBUG_PRINTLN("Setup output pins end");

  stopMotorsNow();

  DEBUG_PRINTLN("Waiting for radio signal");
  while (true) {
    RcInput rc = readRadio();
    if (rc.valid) {
      break;
    }
  }

  DEBUG_PRINTLN("Setup ended");
}

void loop() {
  RcInput rc = readRadio();

  if (!rc.valid) {
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
    return;
  }

  // Debounce ch5 input to avoid rapid toggling of reverse mode
  if (abs(rc.ch5 - lastCh5Value) > 20) {
    ch5DebounceTimer = millis();
    lastCh5Value = rc.ch5;
  }

  //if signal didnt change > 50 ms
  if ((millis() - ch5DebounceTimer) > DEBOUNCE_DELAY_MS) {
    if (stableCh5Value == 0) stableCh5Value = rc.ch5;
    stableCh5Value = rc.ch5;
  }

  bool throttleLow = rc.ch3 <= REVERSE_ARM_MAX_US;
  bool requestedReverse = rc.ch5Ok && stableCh5Value < CH5_REVERSE_THRESHOLD_US;

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

  if (safetyStop) {
    writeMotors(0, 0);
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
    return;
  }

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
}
