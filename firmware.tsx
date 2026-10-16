// The BLE control relay firmware: what it gives (CommandAGI docs/devices.md § firmware). A device definition that
// names this folder as its firmware is held to these channels by `commandagi check <definition.tsx>`.
//
// hid: the relay's one channel. Commands go in on the HID characteristic (CAGI_CHAR_HID, write and write without
// response, src/ble_prov.cpp): the HidControl intents moveTo, move, down, up, click, scroll, type and key
// (src/control.cpp). State comes out on the STATUS characteristic (read and notify, src/status.cpp). Both are on the
// CommandAGI GATT service over BLE, so the channel is duplex. INFO, PROVISION (a friendly name only) and COMMAND
// (identify, reboot, factory-reset) set the unit up; they are not a channel a unit opens.
//
// Revision A: the first hardware, an ESP32-S3 devkit (env esp32s3, the primary) or a classic ESP32 devkit (env esp32).
export default () => (
  <firmware name="CommandAGI control relay">
    <supports revision="A" />
    <channel id="hid" dir="duplex" medium="records" format="hid-control" transport="ble" />
  </firmware>
);
