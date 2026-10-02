#include "telemetryLog.h"

static const char* AP_SSID   = "UGV";
static const char* AP_PASS   = "12345678";
static const char* CSV_PATH  = "/csv/telemetry.csv";
static const char* CSV_DIR   = "/csv";

AsyncFsWebServer server(LittleFS, 80, "ugv");

// Saving last row to avoid writing the same data to flash repeatedly
static String lastLoggedPayload;

static void ensureCsvFile() {
  if (!LittleFS.exists(CSV_DIR)) {
    LittleFS.mkdir(CSV_DIR);
  }
  if (!LittleFS.exists(CSV_PATH)) {
    File f = LittleFS.open(CSV_PATH, "w");
    if (f) {
      f.println(
        "millis,mode,ch1,raw1,ch3,raw3,ch5,raw5,ok1,ok3,ok5,step,"
        "throttleLow,requestedReverse,power,steering,targetL,targetR,"
        "currentL,currentR,reverse,revL,revR"
      );
      f.close();
    }
  }
}

void setupTelemetryLog() {
  if (!LittleFS.begin()) {
    Serial.println("FS mount FAILED, formatting...");
    LittleFS.format();
    ESP.restart();
  }

  ensureCsvFile();

  // Turn on access point with SSID and password
  server.startCaptivePortal(AP_SSID, AP_PASS);

  server.enableFsCodeEditor();
  server.init();

  Serial.print("AP IP: ");
  Serial.println(server.getServerIP());
  Serial.println("Open http://192.168.4.1/index.htm to browse csv logs");
}

void logTelemetryRow(const char* mode, const RcInput &rc,
                      int power, int steering,
                      int leftTarget, int rightTarget,
                      int currentLeftVal, int currentRightVal,
                      bool reverse, bool throttleLow, bool requestedReverse) {

  char payload[200];
  snprintf(payload, sizeof(payload),
    "%s,%d,%lu,%d,%lu,%d,%lu,%d,%d,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    mode,
    rc.ch1, rc.ch1Raw,
    rc.ch3, rc.ch3Raw,
    rc.ch5, rc.ch5Raw,
    rc.ch1Ok, rc.ch3Ok, rc.ch5Ok,
    rc.readStep,
    throttleLow, requestedReverse,
    power, steering,
    leftTarget, rightTarget,
    currentLeftVal, currentRightVal,
    reverse,
    currentLeftVal < 0, currentRightVal < 0
  );

  if (lastLoggedPayload == payload) {
    return;
  }
  lastLoggedPayload = payload;

  File f = LittleFS.open(CSV_PATH, "a");
  if (!f) return;
  f.printf("%lu,%s\n", millis(), payload);
  f.close();
}