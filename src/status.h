#pragma once
#include <Arduino.h>

// The DeviceStatus the relay publishes on the STATUS characteristic (read+notify). For a pure HID
// accessory the states are simpler than a streaming device: idle (advertising, unpaired) → streaming
// (a host is bonded and can receive input) is the whole lifecycle.
namespace Status {
void begin();
// Set the current state ("idle" | "streaming" | "error") + optional detail; notifies subscribers.
void set(const char* state, const char* error = nullptr);
String toJson();
// Register the STATUS characteristic so set() can push notifications.
void attach(void* characteristic);
}  // namespace Status
