#pragma once
#include <Arduino.h>

// Minimal persistent state for the relay — just a friendly device name (the relay carries no Wi-Fi
// creds or account key; it never talks to the cloud). Stored in the `cagi` NVS namespace; a
// factory-reset erases it.
namespace Store {
void begin();
String deviceName();              // stored name, or "" if never set
void setDeviceName(const String& name);
void factoryReset();              // erase the namespace
}  // namespace Store
