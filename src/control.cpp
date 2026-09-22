#include "control.h"
#include <ArduinoJson.h>
#include "hid.h"

namespace {
uint8_t buttonFromJson(JsonVariantConst v) {
  const char* b = v.is<const char*>() ? v.as<const char*>() : "left";
  if (strcmp(b, "right") == 0) return Hid::BTN_RIGHT;
  if (strcmp(b, "middle") == 0) return Hid::BTN_MIDDLE;
  return Hid::BTN_LEFT;
}
}  // namespace

namespace Control {

void handle(const uint8_t* data, size_t len) {
  JsonDocument d;
  if (deserializeJson(d, data, len)) return;  // malformed → drop (the app validated; be defensive)
  const char* t = d["t"];
  if (!t) return;

  if (strcmp(t, "moveTo") == 0) {
    Hid::moveTo((float)(d["x"] | 0.0), (float)(d["y"] | 0.0));
  } else if (strcmp(t, "move") == 0) {
    Hid::moveRel((int)(d["dx"] | 0), (int)(d["dy"] | 0));
  } else if (strcmp(t, "down") == 0) {
    Hid::buttonDown(buttonFromJson(d["button"]));
  } else if (strcmp(t, "up") == 0) {
    Hid::buttonUp(buttonFromJson(d["button"]));
  } else if (strcmp(t, "click") == 0) {
    int count = d["count"] | 1;
    if (count < 1) count = 1;
    if (count > 3) count = 3;
    Hid::click(buttonFromJson(d["button"]), count);
  } else if (strcmp(t, "scroll") == 0) {
    Hid::scroll((int)(d["dx"] | 0), (int)(d["dy"] | 0));
  } else if (strcmp(t, "type") == 0) {
    const char* text = d["text"];
    if (text) Hid::typeText(String(text));
  } else if (strcmp(t, "key") == 0) {
    const char* key = d["key"];
    if (key) Hid::keyChord(String(key));
  }
}

}  // namespace Control
