#include <Arduino.h>
#include "Pins.h"
#include "ctrlFunctions.h"


void ledOn(){
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff(){
  digitalWrite(LED_BUILTIN, LOW);
}
