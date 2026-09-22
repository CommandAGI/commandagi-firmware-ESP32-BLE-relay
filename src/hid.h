#pragma once
#include <Arduino.h>

// The BLE HID device — the relay's output side. Presents a composite HID gadget (relative mouse +
// keyboard + ABSOLUTE pointer) to the paired phone, and exposes emit helpers the control layer calls to
// turn HidControl intents into raw HID reports. Absolute positioning (moveTo/tapAt) is what lets a tap
// land at (x,y) with no knowledge of the current cursor — the app sends normalized 0..1 coordinates.
//
// The custom CommandAGI provisioning GATT service (INFO/STATUS/COMMAND + the HID control characteristic)
// is added to the SAME NimBLE server by BleProv, so one BLE link carries both the OS's HID host role and
// our app's control writes.
namespace Hid {
// Pointer buttons (bitmask for the mouse report).
enum Button { BTN_LEFT = 0x01, BTN_RIGHT = 0x02, BTN_MIDDLE = 0x04 };

// Bring up NimBLE, the HID service + report map (does NOT advertise yet — BleProv adds its service to
// the same server first). Idempotent.
void begin();
// Start HID services + advertising. Call AFTER BleProv::begin() has added the custom service, so both
// the HID service and the CommandAGI control service go live on one BLE link.
void startAdvertising();
// True once a host has bonded and subscribed to the input reports (ready to receive input).
bool isConnected();

// ── emit helpers (called by the control layer) ──────────────────────────────
// Absolute pointer move to a normalized 0..1 screen position (no button change).
void moveTo(float nx, float ny);
// Absolute tap at a normalized point: press + release the given button `count` times (1=tap, 2=double).
void tapAt(float nx, float ny, uint8_t button, int count);
// Relative pointer motion in HID mouse units (boot-protocol fallback).
void moveRel(int dx, int dy);
// Hold / release a button (for drags), and a full click at the current position.
void buttonDown(uint8_t button);
void buttonUp(uint8_t button);
void click(uint8_t button, int count);
// Wheel scroll: +dy scrolls down, +dx scrolls right, in HID wheel detents.
void scroll(int dx, int dy);
// Type a UTF-8 string as a sequence of key presses (US layout best-effort).
void typeText(const String& text);
// Press a key chord by name, e.g. "Return", "ctrl+shift+t", "cmd+space".
void keyChord(const String& name);
}  // namespace Hid
