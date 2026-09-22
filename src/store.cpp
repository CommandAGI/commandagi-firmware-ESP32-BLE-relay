#include "store.h"
#include <Preferences.h>
#include "config.h"

namespace {
Preferences g_prefs;
}

namespace Store {

void begin() { g_prefs.begin(CAGI_NVS_NAMESPACE, false); }

String deviceName() { return g_prefs.getString("name", ""); }

void setDeviceName(const String& name) { g_prefs.putString("name", name); }

void factoryReset() { g_prefs.clear(); }

}  // namespace Store
