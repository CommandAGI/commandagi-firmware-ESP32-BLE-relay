#include "ble_prov.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "config.h"
#include "store.h"
#include "status.h"
#include "control.h"

namespace {
NimBLECharacteristic* g_info = nullptr;

String fullMac() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[13];
  for (int i = 0; i < 6; i++) snprintf(buf + i * 2, 3, "%02X", (uint8_t)((mac >> (8 * i)) & 0xFF));
  return String(buf);
}
String macSuffix() {
  String mac = fullMac();
  return mac.length() >= 4 ? mac.substring(mac.length() - 4) : mac;
}

String defaultName() { return String(CAGI_ADV_NAME_PREFIX) + macSuffix(); }

String infoJson() {
  JsonDocument d;
  d["kind"] = "relay";  // the ONE field that distinguishes it from a camera/arm on the shared service
  d["model"] = CAGI_MODEL;
  d["fw"] = CAGI_FW_VERSION;
  d["hwid"] = fullMac();
  String name = Store::deviceName();
  d["name"] = name.length() ? name : defaultName();
  d["provisioned"] = name.length() > 0;  // "named" — the relay has no creds to be provisioned with
  d["secure"] = false;                    // no PIN-sealed provisioning (no secrets travel over BLE)
  String out;
  serializeJson(d, out);
  return out;
}

void blinkIdentify() {
  pinMode(CAGI_LED_PIN, OUTPUT);
  for (int i = 0; i < 6; i++) {
    digitalWrite(CAGI_LED_PIN, HIGH);
    delay(120);
    digitalWrite(CAGI_LED_PIN, LOW);
    delay(120);
  }
}

// PROVISION: plaintext { deviceName } only (the relay stores no Wi-Fi/account secrets).
class ProvisionCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* ch) override {
    JsonDocument d;
    if (deserializeJson(d, ch->getValue().c_str())) return;
    const char* name = d["deviceName"];
    if (name && strlen(name)) {
      Store::setDeviceName(String(name));
      if (g_info) g_info->setValue(infoJson());
    }
  }
};

class CommandCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* ch) override {
    String cmd = ch->getValue().c_str();
    cmd.trim();
    if (cmd == "identify") {
      blinkIdentify();
    } else if (cmd == "reboot") {
      delay(200);
      ESP.restart();
    } else if (cmd == "factory-reset") {
      Store::factoryReset();
      delay(200);
      ESP.restart();
    }
  }
};

// HID control: a HidControl intent JSON → drive the HID device.
class HidControlCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* ch) override {
    std::string v = ch->getValue();
    Control::handle((const uint8_t*)v.data(), v.size());
  }
};
}  // namespace

namespace BleProv {

String hwid() { return fullMac(); }
String hwSuffix() { return macSuffix(); }

void begin() {
  NimBLEServer* server = NimBLEDevice::getServer();
  if (!server) return;  // Hid::begin() must have created it first

  NimBLEService* svc = server->createService(CAGI_SVC_UUID);

  g_info = svc->createCharacteristic(CAGI_CHAR_INFO, NIMBLE_PROPERTY::READ);
  g_info->setValue(infoJson());

  NimBLECharacteristic* status =
      svc->createCharacteristic(CAGI_CHAR_STATUS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  status->setValue(Status::toJson());
  Status::attach(status);

  NimBLECharacteristic* prov = svc->createCharacteristic(CAGI_CHAR_PROVISION, NIMBLE_PROPERTY::WRITE);
  prov->setCallbacks(new ProvisionCb());

  NimBLECharacteristic* cmd = svc->createCharacteristic(CAGI_CHAR_COMMAND, NIMBLE_PROPERTY::WRITE);
  cmd->setCallbacks(new CommandCb());

  // HID control — accept both write-with-response and write-without-response (the app uses the latter
  // for low-latency pointer motion).
  NimBLECharacteristic* hid =
      svc->createCharacteristic(CAGI_CHAR_HID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  hid->setCallbacks(new HidControlCb());

  svc->start();
}

}  // namespace BleProv
