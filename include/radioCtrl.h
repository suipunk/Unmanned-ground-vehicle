#pragma once

#include "RadioConfig.h"

extern RcChannelState ch1State;
extern RcChannelState ch3State;
extern RcChannelState ch5State;
extern uint8_t rcReadStep;

void updateRcChannel(uint8_t pin, RcChannelState *channel);
void validateRadioSignal();
RcInput readRadio();