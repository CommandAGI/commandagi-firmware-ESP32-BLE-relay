#pragma once
#include <Arduino.h>

// Parse one HidControl intent (the JSON written to the HID characteristic) and drive the HID device.
// The wire schema is packages/domain/core/src/deviceProvisioning.ts `HidControl`:
//   { t:"moveTo", x, y } | { t:"move", dx, dy } | { t:"down"|"up", button } |
//   { t:"click", button, count } | { t:"scroll", dx, dy } | { t:"type", text } | { t:"key", key }
namespace Control {
void handle(const uint8_t* data, size_t len);
}
