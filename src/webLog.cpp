#include "webLog.h"

static const char* HOSTNAME = "ugv-log";

AsyncFsWebServer server(FILESYSTEM, 80, HOSTNAME);
MultiPrint multiLog;

// ---- MultiPrint ----

size_t MultiPrint::write(uint8_t c) {
  Serial.write(c);
  _buffer += (char)c;

  if (c == '\n' || _buffer.length() > 200) {
    flushLine();
  }
  return 1;
}

size_t MultiPrint::write(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; i++) {
    write(buffer[i]);
  }
  return size;
}

void MultiPrint::flushLine() {
  server.wsBroadcast(_buffer.c_str());
  _buffer = "";
}

// ---- WebSocket events ----

void onWsEvent(AsyncWebSocket *ws, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Client #%u disconnected\n", client->id());
  }
}

// ---- Public API ----

void setupWebLog() {
  if (!FILESYSTEM.begin()) {
    Serial.println("FS mount FAILED, formatting...");
    FILESYSTEM.format();
    ESP.restart();
  }

  if (!server.startWiFi(10000)) {
    Serial.println("WiFi not connected! Starting AP mode...");
    server.startCaptivePortal("UGV_AP", "12345678");
  }

  server.enableWebSocket("/ws", onWsEvent);

  server.enableFsCodeEditor();

  server.init();

  Serial.print("Web log server started, IP: ");
  Serial.println(server.getServerIP());
}

void handleWebLog() {
  if (server.isAccessPointMode()) {
    server.updateDNS();
  }

  AsyncWebSocket* ws = server.getWebSocket();
  if (ws) {
    ws->cleanupClients();
  }
}