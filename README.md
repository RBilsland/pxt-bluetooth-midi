# bluetooth-midi [![Build Status](https://travis-ci.org/Microsoft/pxt-bluetooth-midi.svg?branch=master)](https://travis-ci.org/Microsoft/pxt-bluetooth-midi)

A [Bluetooth low energy MIDI](https://www.midi.org/specifications/item/bluetooth-le-midi) 
support for the @boardname@.
**iOS and macOS** (via Audio MIDI Setup Bluetooth MIDI).

### ~hint
![](/static/bluetooth/Bluetooth_SIG.png)

For another device like a smartphone to use any of the Bluetooth "services" which the @boardname@ has, it must first be [paired with the @boardname@](/reference/bluetooth/bluetooth-pairing). Once paired, the other device may connect to the @boardname@ and exchange data relating to many of the @boardname@'s features.

### ~

## Usage

This package allows the @boardname@ to act as a MIDI peripherical, like a piano. It requires to connect to a BLE MIDI device to receive the commands and play them.

This library uses the [MIDI package](/pkg/microsoft/pxt-midi). 
Please refer to that project documentation for further details on using MIDI commands.

## Supported Platforms

This package supports **iOS** (iPad/iPhone) and **macOS** (Audio MIDI Setup → Bluetooth configuration, Central role).
Any app that supports a BLE MIDI keyboard should work.

### macOS Audio MIDI Setup

1. Open **Audio MIDI Setup** → **Window** → **Show MIDI Studio**.
2. Double-click the **Bluetooth** configuration icon.
3. In the **Central** section (bottom), wait for your @boardname@ to appear, select it, and click **Connect**.
4. The device should then show as a MIDI input in other apps.

**Important for Mac:** use **Just Works pairing** (MakeCode default), **not** “No Pairing Required”. macOS Core MIDI expects an encrypted BLE link; open/unencrypted mode can connect in nRF Connect but still not appear in the MIDI Studio Bluetooth list.

You must flash **v2.0.18** or later on a **micro:bit v2**: v2.0.17 always advertises the **full 128-bit** MIDI UUID (`0x07` AD type). Older builds sometimes advertised only a 16-bit alias (visible in nRF Connect **after** connect, but invisible to macOS MIDI scan). (micro:bit v1 + macOS is not supported for discovery.)

### Apple GarageBand

* [iPhone/iPad App](https://itunes.apple.com/us/app/garageband/id408709785?mt=8)

Go to settings (gearwheel), click **Advanced**, click **Bluetooth MIDI device** and connect to the @boardname@.
If the @boardname@ is marked as offline, click **Edit** and **Forget** the device.

## micro:bit v2

micro:bit v2 uses the **CODAL** runtime, not the v1 **DAL/mbed** stack. Older releases of this extension only implemented BLE MIDI with mbed APIs, so they **did not compile for v2**. MakeCode then disabled the package for v2 boards and showed **[error 929](https://support.microbit.org/support/solutions/articles/19000121371-makecode-extension-compatibility-v1-and-v2)** (“extension not compatible with this board”).

From **v2.0.14** onward, the extension builds on both v1 and v2: v2 uses the same CODAL BLE service model as the built-in Bluetooth blocks. **v2.0.15+** advertises the MIDI service UUID; **v2.0.16** fixes connection timeouts; **v2.0.17** fixes macOS MIDI scan (128-bit UUID in advertisements + Just Works security); **v2.0.18** adds the full BLE MIDI characteristic (read/write/notify) and faster connection interval for macOS connect. If you still see error 929, remove and re-add the extension (or use this repo version) and download a fresh `.hex` after upgrading.

## Troubleshooting BLE scanners (Android / nRF Connect)

### The device does not show up when scanning

This extension’s firmware defaults to **Just Works pairing, no whitelist** so MIDI centrals (especially macOS) can discover and encrypt the link. If you previously chose **“No Pairing Required”** for Android testing, switch back to **Just Works** for Mac MIDI Studio, then download a new `.hex`.

If the board is missing from scans entirely, try **pairing mode** once: hold **A + B**, press **reset**, release reset (keep A + B until “PAIRING MODE” scrolls), then scan again.

Bluetooth must be enabled in the firmware: adding this extension pulls in the `bluetooth` package, which turns the stack on. You do **not** need separate “start accelerometer” blocks for MIDI to work.

### Scanner sees the micro:bit but connection times out

That was a bug in **v2.0.15**: the board advertised the MIDI service but used a **whitelist connection filter** while no devices were bonded, so Android/macOS could see it but not connect. **v2.0.16** fixes this (always accepts connections for advertising) and encodes the correct service UUID type. Flash **v2.0.16+** and forget/remove old “micro:bit” entries on the phone/Mac before retrying.

### I see the micro:bit but not the MIDI service UUID

That is normal. BLE **scan results** only show advertising data (name, sometimes a few UUIDs). Custom **GATT services are not listed while scanning**; they appear only **after you connect** as a GATT client:

1. In **nRF Connect** (or similar), **connect** to the micro:bit (do not rely on scan-only view).
2. Open **Services** / **Discover services** (use “refresh” if you connected before).
3. Look for service **`03B80E5A-EDE8-4B33-A751-6CE34EC4C700`** (MIDI) and characteristic **`7772E5DB-3868-412A-A1A9-F2669D106BF3`**.

If you connected earlier, **disconnect**, **forget** the device in Android Bluetooth settings, refresh services in the app, and reconnect so the phone does not use a cached GATT table from an old program.

### GarageBand / iOS still cannot see MIDI

iOS discovers BLE MIDI through the app (Settings → Advanced → Bluetooth MIDI), not a generic scanner. Pair or use **No Pairing Required**, flash the latest hex, and ensure the program runs (the extension starts the MIDI service automatically on boot).

## Supported targets

* for PXT/microbit (micro:bit v1 and v2)
* for PXT/calliope

(The metadata above is needed for package search.)

## License

MIT

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
