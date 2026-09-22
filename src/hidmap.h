#pragma once
#include <Arduino.h>

// Map key names + UTF-8 chars to USB HID keyboard usage codes (US layout, best-effort). Mirrors the
// xdotool-style key names host-core uses elsewhere (packages/host-node key-map), so "Return",
// "ctrl+shift+t", "cmd+space" all resolve. The `mod` byte is the HID modifier bitmask (LCtrl 0x01,
// LShift 0x02, LAlt 0x04, LGui 0x08).
namespace HidMap {
// A single printable char → (modifier, keycode). Returns false for chars with no US-layout mapping.
bool charToKey(uint8_t c, uint8_t& mod, uint8_t& key);
// A key chord name ("Return", "ctrl+shift+t", "a", "cmd+space") → (modifier, keycode). False if unknown.
bool nameToKey(const String& name, uint8_t& mod, uint8_t& key);
}  // namespace HidMap
