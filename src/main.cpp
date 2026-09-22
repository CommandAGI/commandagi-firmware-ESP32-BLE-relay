// CommandAGI BLE control-relay firmware — entry point.
//
// Boot → advertise as a BLE HID pointer/keyboard AND the custom CommandAGI service. A phone pairs the
// relay as a Bluetooth input device (system pairing) and our app connects to the custom service and
// writes HidControl intents; the relay turns them into HID reports the phone accepts system-wide.
//
// There is no Wi-Fi, no cloud, no camera — a pure BLE HID accessory. See README.md / NOTES.md.
#include <Arduino.h>
#include "ble_prov.h"
#include "config.h"
#include "hid.h"
#include "status.h"
#include "store.h"

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("CommandAGI Control Relay v%s — hwid %s\n", CAGI_FW_VERSION, BleProv::hwid().c_str());

  Store::begin();
  Status::begin();

  // Order matters: Hid::begin() inits NimBLE + creates the server + HID device; BleProv::begin() adds
  // the custom control service to that SAME server; Hid::startAdvertising() starts services + advertising
  // so both the HID service and the CommandAGI service are live on one link.
  Hid::begin();
  BleProv::begin();
  Hid::startAdvertising();

  Status::set("idle");
  Serial.println("Advertising — pair me as a Bluetooth device, then connect in the CommandAGI app.");
}

void loop() {
  // Reflect the HID host connection state on the STATUS characteristic. "streaming" = a host is bonded
  // and can receive input (the relay's equivalent of a live device).
  static bool wasConnected = false;
  bool now = Hid::isConnected();
  if (now != wasConnected) {
    wasConnected = now;
    Status::set(now ? "streaming" : "idle");
    Serial.println(now ? "Host connected — relaying input." : "Host disconnected — advertising.");
  }
  delay(50);
}
