#include "hid.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "config.h"
#include "hidmap.h"

namespace {
NimBLEHIDDevice* g_hid = nullptr;
NimBLECharacteristic* g_mouse = nullptr;  // report id 1: relative mouse
NimBLECharacteristic* g_keyboard = nullptr;  // report id 2: keyboard
NimBLECharacteristic* g_abs = nullptr;  // report id 3: absolute pointer
volatile bool g_connected = false;
uint8_t g_buttons = 0;  // held mouse buttons (for drags)

// Composite HID report map: relative mouse (ID 1), keyboard (ID 2), absolute mouse (ID 3). The absolute
// report drives a system pointer at a given (x,y) — iOS AssistiveTouch + Android both accept an absolute
// pointing device. See NOTES.md for per-OS report-map tuning caveats.
const uint8_t REPORT_MAP[] = {
    // ── Report ID 1: relative mouse ─────────────────────────────────────────
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x03,        //     Input (Const,Var,Abs) padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel)
    0xC0,              //   End Collection
    0xC0,              // End Collection

    // ── Report ID 2: keyboard ───────────────────────────────────────────────
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (224)
    0x29, 0xE7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs) modifier byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Const) reserved
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data,Array) 6 keys
    0xC0,              // End Collection

    // ── Report ID 3: absolute mouse (system pointer) ────────────────────────
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x03,        //     Input (Const) padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x02,        //     Input (Data,Var,Abs) ABSOLUTE X/Y
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

class ServerCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) override {
    // HID hosts require encryption before they accept reports; request it immediately.
    NimBLEDevice::startSecurity(desc->conn_handle);
  }
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    g_connected = desc->sec_state.encrypted;
  }
  void onDisconnect(NimBLEServer* server, ble_gap_conn_desc* desc) override {
    g_connected = false;
    g_buttons = 0;
    NimBLEDevice::startAdvertising();  // become pairable again
  }
};

uint16_t clampAbs(float n) {
  if (n < 0) n = 0;
  if (n > 1) n = 1;
  return (uint16_t)(n * 32767.0f);
}

int8_t clampRel(int v) { return (int8_t)(v < -127 ? -127 : v > 127 ? 127 : v); }

void sendMouse(uint8_t buttons, int dx, int dy, int wheel) {
  if (!g_mouse) return;
  uint8_t report[4] = {buttons, (uint8_t)clampRel(dx), (uint8_t)clampRel(dy), (uint8_t)clampRel(wheel)};
  g_mouse->setValue(report, sizeof(report));
  g_mouse->notify();
}

void sendAbs(uint8_t buttons, uint16_t x, uint16_t y) {
  if (!g_abs) return;
  uint8_t report[5] = {buttons, (uint8_t)(x & 0xFF), (uint8_t)(x >> 8), (uint8_t)(y & 0xFF), (uint8_t)(y >> 8)};
  g_abs->setValue(report, sizeof(report));
  g_abs->notify();
}

void sendKey(uint8_t modifier, uint8_t keycode) {
  if (!g_keyboard) return;
  uint8_t report[8] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
  g_keyboard->setValue(report, sizeof(report));
  g_keyboard->notify();
}
void releaseKeys() { sendKey(0, 0); }
}  // namespace

namespace Hid {

void begin() {
  if (g_hid) return;

  String advName = String(CAGI_ADV_NAME_PREFIX);
  {
    uint64_t mac = ESP.getEfuseMac();
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", (uint8_t)((mac >> 8) & 0xFF), (uint8_t)(mac & 0xFF));
    advName += suffix;
  }
  NimBLEDevice::init(advName.c_str());
  NimBLEDevice::setMTU(CAGI_MTU);

  // HID hosts require bonding. Just-Works by default (no passkey UI); a unit with a printed passkey can
  // force MITM via CAGI_BLE_PASSKEY.
  bool mitm = CAGI_BLE_PASSKEY != 0;
  NimBLEDevice::setSecurityAuth(true, mitm, true);  // bond, MITM?, secure connections
  if (mitm) {
    NimBLEDevice::setSecurityPasskey(CAGI_BLE_PASSKEY);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  } else {
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  }

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCb());

  g_hid = new NimBLEHIDDevice(server);
  g_hid->manufacturer()->setValue("CommandAGI");
  g_hid->pnp(0x02, 0xE502, 0x0001, 0x0110);  // vendor-defined PnP id
  g_hid->hidInfo(0x00, 0x01);                // country 0, remote-wake

  g_mouse = g_hid->inputReport(1);
  g_keyboard = g_hid->inputReport(2);
  g_abs = g_hid->inputReport(3);
  g_hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));

  // BleProv adds the custom CommandAGI service (INFO/STATUS/COMMAND/HID-control) to this same server
  // before we start services + advertising (see main.cpp ordering).
}

// Called by main after BleProv has added its service — starts HID services + advertising together so
// both the HID service and the custom CommandAGI service are live on one link.
void startAdvertising() {
  g_hid->startServices();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(0x03C2);  // HID Mouse
  adv->addServiceUUID(g_hid->hidService()->getUUID());  // 0x1812 — OS recognizes a HID pointer
  adv->addServiceUUID(NimBLEUUID(CAGI_SVC_UUID));       // our app filters on this
  adv->setScanResponse(true);
  NimBLEDevice::startAdvertising();
}

bool isConnected() { return g_connected; }

void moveTo(float nx, float ny) { sendAbs(g_buttons, clampAbs(nx), clampAbs(ny)); }

void tapAt(float nx, float ny, uint8_t button, int count) {
  uint16_t x = clampAbs(nx), y = clampAbs(ny);
  for (int i = 0; i < count; i++) {
    sendAbs(button, x, y);  // tip/button down at (x,y)
    delay(15);
    sendAbs(0, x, y);       // up
    if (i + 1 < count) delay(60);
  }
}

void moveRel(int dx, int dy) { sendMouse(g_buttons, dx, dy, 0); }

void buttonDown(uint8_t button) {
  g_buttons |= button;
  sendMouse(g_buttons, 0, 0, 0);
}
void buttonUp(uint8_t button) {
  g_buttons &= ~button;
  sendMouse(g_buttons, 0, 0, 0);
}
void click(uint8_t button, int count) {
  for (int i = 0; i < count; i++) {
    sendMouse(g_buttons | button, 0, 0, 0);
    delay(15);
    sendMouse(g_buttons & ~button, 0, 0, 0);
    if (i + 1 < count) delay(60);
  }
}

void scroll(int dx, int dy) {
  // HID wheel is +up / -down; our contract is +dy = down, so negate.
  sendMouse(g_buttons, 0, 0, -dy);
}

void typeText(const String& text) {
  for (size_t i = 0; i < text.length(); i++) {
    uint8_t mod = 0, key = 0;
    if (HidMap::charToKey((uint8_t)text[i], mod, key) && key) {
      sendKey(mod, key);
      delay(8);
      releaseKeys();
      delay(8);
    }
  }
}

void keyChord(const String& name) {
  uint8_t mod = 0, key = 0;
  if (!HidMap::nameToKey(name, mod, key)) return;
  sendKey(mod, key);
  delay(12);
  releaseKeys();
}

}  // namespace Hid
