#pragma once
#include <Arduino.h>

// The custom CommandAGI GATT service — the relay's control side. Added to the SAME NimBLE server the
// HID device uses (Hid::begin() creates the server; call BleProv::begin() after it, then
// Hid::startAdvertising()). Exposes:
//   INFO (read)        — DeviceInfo JSON with kind "relay", so the app recognizes it.
//   STATUS (read+notify)
//   PROVISION (write)  — optional plaintext { deviceName } (the relay stores no secrets).
//   COMMAND (write)    — identify | reboot | factory-reset.
//   HID control (write)— HidControl intent JSON → Control::handle() → HID reports.
namespace BleProv {
void begin();       // add the service to the existing server (idempotent)
String hwid();      // full eFuse MAC
String hwSuffix();  // 4-hex suffix
}  // namespace BleProv
