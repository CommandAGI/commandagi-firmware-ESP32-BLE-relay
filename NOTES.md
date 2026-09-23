# Notes — honest caveats & next steps (untested on hardware)

This firmware is written to the NimBLE-Arduino API and the shared `HidControl` wire contract, but has
**not yet been flashed to a board**. The pieces most likely to need real-hardware tuning:

1. **Report-map acceptance varies by OS.** iOS/Android accept a standard HID mouse/keyboard readily; the
   **absolute-pointer** report (Report ID 3) is the part to validate first. If a host ignores absolute
   positioning, options in order of preference: (a) tweak the descriptor (some hosts want a Digitizer
   usage page `0x0D` instead of an absolute Generic-Desktop mouse), (b) fall back to relative motion —
   the app already emits `move` deltas, and `controlActionToHid` can be pointed at relative mode. iOS
   AssistiveTouch specifically is the acceptance test for "tap at (x,y)".

2. **Pairing + bonding.** HID hosts require an encrypted, bonded link before accepting input reports.
   We default to **Just-Works** bonding (`CAGI_BLE_REQUIRE_BONDING=1`, passkey `0`) for a one-tap pair.
   If a unit has a printed label, set `-DCAGI_BLE_PASSKEY=NNNNNN` to force a MITM passkey prompt. Confirm
   the app can still write the HID-control characteristic once bonded (same link, GATT client role).

3. **One link, two roles.** The phone is simultaneously the **HID host** (system pairing) and our app's
   **GATT client** (writing intents). This works on one BLE connection because both the HID service and
   the custom CommandAGI service live on the same NimBLE server. Verify on both platforms that the app's
   GATT writes are permitted over the bonded HID link (they should be; harden by requiring `WRITE_ENC`
   on the HID characteristic once confirmed).

4. **Security hardening (post-bring-up).** Consider `WRITE_ENC` on the HID-control characteristic so only
   the bonded host can inject, and a short idle auto-disconnect. The relay carries no secrets, so the
   threat model is "someone nearby injects input" — bonding already gates that.

5. **Latency.** Pointer motion uses write-without-response for low latency. If motion feels stepped, tune
   the app's send cadence and/or coalesce `move` deltas; the firmware emits one report per intent.

6. **Board LED.** `CAGI_LED_PIN` defaults to GPIO48 (S3 RGB) / GPIO2 (classic). Override per board.

## Promotion to a standalone repo

Like the camera (`commandagi-camera`) and arm (`commandagi-robot-firmware`), this is intended to become
its own repo added as a submodule at `apps/clients/firmware/ESP32-BLE-relay`. It currently lives in-tree (implemented
cautiously alongside concurrent work). To promote:

```bash
gh repo create CommandAGI/commandagi-ble-relay --private --source apps/clients/firmware/ESP32-BLE-relay --push
# then in the monorepo: remove the tracked dir and `git submodule add` the new URL at the same path
```
