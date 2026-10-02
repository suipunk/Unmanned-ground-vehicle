#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <AsyncFsWebServer.h>
#include "RadioConfig.h"   // RcInput

// Mounts FS, creates the /csv folder, and starts logginh
void setupTelemetryLog();

// Writes a single CSV rowwhen if the data differs from the previous
// successfully written row (without being tied to a timer).
void logTelemetryRow(const char* mode, const RcInput &rc,
                      int power, int steering,
                      int leftTarget, int rightTarget,
                      int currentLeftVal, int currentRightVal,
                      bool reverse, bool throttleLow, bool requestedReverse);