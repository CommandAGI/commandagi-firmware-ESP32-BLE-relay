#include "status.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>

namespace {
String g_state = "idle";
String g_error = "";
NimBLECharacteristic* g_char = nullptr;
}  // namespace

namespace Status {

void begin() {
  g_state = "idle";
  g_error = "";
}

void attach(void* characteristic) { g_char = (NimBLECharacteristic*)characteristic; }

String toJson() {
  JsonDocument d;
  d["state"] = g_state;
  if (g_error.length()) d["error"] = g_error;
  String out;
  serializeJson(d, out);
  return out;
}

void set(const char* state, const char* error) {
  g_state = state;
  g_error = error ? error : "";
  if (g_char) {
    g_char->setValue(toJson());
    g_char->notify();
  }
}

}  // namespace Status
