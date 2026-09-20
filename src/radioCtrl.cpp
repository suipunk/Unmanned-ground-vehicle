#include "Pins.h"
#include "RadioConfig.h"
#include "radioCtrl.h"
#include "debugPrint.h"
#include "ctrlFunctions.h"

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

void validateRadioSignal()
{
  DEBUG_PRINTLN("Waiting for radio signal");
  RcInput rc = readRadio();
  rc.valid = false;
  uint32_t debugStartMs = millis();
  bool ledState = false;
  while (true) {
    rc = readRadio();
    if (rc.valid) {
      break;
    }else{
      if(millis() - debugStartMs >= DEBUG_INTERVAL_MS){
        debugStartMs = millis();
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW); // todo in function switchLedState();
        DEBUG_PRINTLN("Waiting for radio signals");
      }
    }
    yield();
  }
  ledOff();
  DEBUG_PRINTLN("Radio signal detected");
}