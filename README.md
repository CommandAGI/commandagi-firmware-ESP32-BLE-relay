# ESP32-BLE-relay

Part of [CommandAGI](https://commandagi.com): connecting agents to real computers, robots and
physical environments. This repository can be cloned independently of the private platform code.

```sh
git clone https://github.com/CommandAGI/commandagi-firmware-ESP32-BLE-relay.git
cd commandagi-firmware-ESP32-BLE-relay
```

## Validation scope

The PlatformIO environments declare supported build targets. A build does not prove phone pairing,
OS accessibility configuration or physical input behavior; record the phone/OS and board revision
when validating those paths. This relay implements Bluetooth HID and does not require cloud access.


The **control relay** is the cross-app / iOS pointer for CommandAGI. It's a small BLE accessory (ESP32)
that pairs to a phone as a standard **Bluetooth HID mouse + keyboard + absolute pointer**, so an agent
can drive the phone's UX **system-wide** — in games, DRM'd views, and other apps that have no
accessibility tree, and on **iOS**, where a third-party app cannot synthesize touch into other apps but
the OS _does_ honor a paired Bluetooth pointer (AssistiveTouch).

It is the hardware counterpart to the on-device Android `AccessibilityService` path:
where accessibility works on-device (Android apps that expose a tree), the relay instead
sends standard HID reports. Host acceptance still needs the hardware checks in [NOTES.md](NOTES.md).

## How it works

```
 CommandAGI app  ──BLE GATT write (HidControl JSON)──▶  relay  ──BLE HID reports──▶  phone OS pointer
 (GATT client)                                              (this firmware)       (system-wide)
```

- The relay advertises the **shared CommandAGI provisioning service** (`c0a1…0001`), so the app
  recognizes it in a scan; `INFO.kind` is `"relay"` (the only thing distinguishing it from a camera/arm).
- It ALSO advertises the standard **HID service** (`0x1812`) + a HID-Mouse appearance, so the phone's OS
  offers to pair it as a Bluetooth input device.
- The app connects and writes **`HidControl`** intents (the wire schema in
  [src/control.cpp](src/control.cpp), with UUIDs in [src/config.h](src/config.h)) to the HID characteristic (`c0a1…0006`). The firmware maps
  each intent onto a raw HID report:
  - `moveTo {x,y}` → absolute-pointer report (normalized 0..1 → 0..32767)
  - `click / down / up / move / scroll` → relative-mouse report
  - `type {text}` / `key {name}` → keyboard reports (US layout, `src/hidmap.cpp`)

No Wi-Fi, no cloud, no account — a pure BLE HID accessory. (`PROVISION` optionally sets a friendly
name; nothing secret ever travels over BLE, so there is no PIN-sealing like the camera.)

## Build / flash

PlatformIO. Primary target is the ESP32-S3 devkit (native USB); the classic ESP32 devkit is an alt env.

```bash
pio run -e esp32s3 -t upload && pio device monitor   # ESP32-S3 devkit
pio run -e esp32   -t upload                          # classic ESP32 devkit
```

Then, on the phone: **Settings → Bluetooth → pair "CommandAGI Relay XXXX"**, open the CommandAGI app →
_Make this phone available → Control my device → Connect a control relay_.

## Layout

| File                   | Role                                                                                     |
| ---------------------- | ---------------------------------------------------------------------------------------- |
| `src/config.h`         | BLE UUIDs, names, security |
| `src/hid.{h,cpp}`      | NimBLE HID device — report map (mouse + keyboard + absolute pointer) + emit helpers      |
| `src/hidmap.{h,cpp}`   | key-name / UTF-8 char → USB HID usage codes (US layout)                                  |
| `src/control.{h,cpp}`  | parse a `HidControl` intent JSON → HID emits                                             |
| `src/ble_prov.{h,cpp}` | the custom CommandAGI GATT service (INFO/STATUS/PROVISION/COMMAND/HID)                   |
| `src/store.{h,cpp}`    | NVS — the friendly name (no secrets)                                                     |
| `src/status.{h,cpp}`   | the STATUS characteristic                                                                |
| `src/main.cpp`         | boot + connection-state reporting                                                        |

See `NOTES.md` for the honest hardware caveats (report-map tuning per OS, pairing, security hardening).

## License

[MIT](LICENSE).

## Build verification (2026-09-22)

All PlatformIO environments declared by this repository compiled successfully using PlatformIO
6.2.0. This verifies compilation, not physical wiring, sensor operation or live cloud connectivity.
