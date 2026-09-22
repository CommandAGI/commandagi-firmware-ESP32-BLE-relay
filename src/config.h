#pragma once
// Compile-time configuration for the CommandAGI BLE control-relay firmware.

// Firmware version reported over BLE (INFO.fw) — bump on each release.
#define CAGI_FW_VERSION "0.1.0"

// Human model string reported over BLE (INFO.model).
#ifndef CAGI_MODEL
#define CAGI_MODEL "CommandAGI Control Relay"
#endif

// ── BLE GATT contract ───────────────────────────────────────────────────────────────────────────
// MUST stay in sync with packages/domain/core/src/deviceProvisioning.ts (the shared source of truth). The
// relay reuses the SAME provisioning service UUID as cameras + the arm so the apps recognize it in a
// scan ("a CommandAGI device"); only INFO.kind ("relay") distinguishes it. The HID characteristic
// (…0006) is relay-only — the app writes HidControl intents to it.
#define CAGI_SVC_UUID       "c0a1d61c-0001-4a17-9c0a-1d61cab00001"
#define CAGI_CHAR_INFO      "c0a1d61c-0002-4a17-9c0a-1d61cab00001"  // read        — DeviceInfo JSON (kind "relay")
#define CAGI_CHAR_STATUS    "c0a1d61c-0003-4a17-9c0a-1d61cab00001"  // read+notify — DeviceStatus JSON
#define CAGI_CHAR_PROVISION "c0a1d61c-0004-4a17-9c0a-1d61cab00001"  // write       — optional { deviceName } (plaintext)
#define CAGI_CHAR_COMMAND   "c0a1d61c-0005-4a17-9c0a-1d61cab00001"  // write       — identify | reboot | factory-reset
#define CAGI_CHAR_HID       "c0a1d61c-0006-4a17-9c0a-1d61cab00001"  // write       — HidControl intent JSON
#define CAGI_MTU            512

// BLE advertised name prefix (a 4-hex MAC suffix is appended, e.g. "CommandAGI Relay A1B2"). This is
// the name the phone shows when pairing it as a Bluetooth HID device, and what our app filters on.
#define CAGI_ADV_NAME_PREFIX "CommandAGI Relay "

// ── HID host security ─────────────────────────────────────────────────────────────────────────────
// HID-over-GATT hosts (iOS/Android) require an encrypted, bonded link before they'll accept input
// reports. So unlike the camera (which defaults bonding off), the relay REQUIRES bonding — "Just
// Works" pairing (no passkey UI) is enough for a pointer and keeps the pair flow one tap. Set a
// passkey build flag to force MITM if a unit has a display/label.
#define CAGI_BLE_REQUIRE_BONDING 1
#ifndef CAGI_BLE_PASSKEY
#define CAGI_BLE_PASSKEY 0            // 0 ⇒ Just-Works pairing (no passkey prompt)
#endif

// The relay carries no Wi-Fi password or account secret over BLE (it never talks to the cloud), so the
// PIN-sealed provisioning the camera uses is unnecessary. INFO.secure is reported false; PROVISION, if
// used at all, only sets a friendly name in plaintext.
#define CAGI_PROV_SECURE 0

// ── NVS (persistent friendly name) ────────────────────────────────────────────────────────────────
#define CAGI_NVS_NAMESPACE "cagi"    // a factory-reset erases exactly this namespace

// On-board LED used as the "identify" indicator (blinks on the identify command). ESP32-S3 devkit
// on-board RGB is GPIO48; classic ESP32 devkit LED is GPIO2. Override with -DCAGI_LED_PIN=<n>.
#ifndef CAGI_LED_PIN
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define CAGI_LED_PIN 48
  #else
    #define CAGI_LED_PIN 2
  #endif
#endif
