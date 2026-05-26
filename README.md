# bluetooth-midi [![Build Status](https://travis-ci.org/Microsoft/pxt-bluetooth-midi.svg?branch=master)](https://travis-ci.org/Microsoft/pxt-bluetooth-midi)

A [Bluetooth low energy MIDI](https://www.midi.org/specifications/item/bluetooth-le-midi) 
support for the @boardname@.
**iOS, macOS, Android, and Windows** (via BLE MIDI apps; see **Supported Platforms**).

### ~hint
![](/static/bluetooth/Bluetooth_SIG.png)

For another device like a smartphone to use any of the Bluetooth "services" which the @boardname@ has, it must first be [paired with the @boardname@](/reference/bluetooth/bluetooth-pairing). Once paired, the other device may connect to the @boardname@ and exchange data relating to many of the @boardname@'s features.

### ~

## Usage

This package allows the @boardname@ to act as a MIDI peripherical, like a piano. It requires to connect to a BLE MIDI device to receive the commands and play them.

This library uses the [MIDI package](/pkg/microsoft/pxt-midi). 
Please refer to that project documentation for further details on using MIDI commands.

## MIDI blocks

Most blocks under **MIDI** come from [pxt-midi](https://github.com/microsoft/pxt-midi), not from bluetooth-midi itself. This extension only adds the Bluetooth transport so those messages reach a phone, tablet, or Mac.

Use **`midi channel` _N_** first; blocks like notes and control change apply to that channel (1–16).

### Control change _controller_ to _value_

Sends a **MIDI Control Change (CC)** message. It does not play a note; it tells the connected app or instrument to adjust a setting (volume, effects, pedals, and so on).

| Block input | Range | Meaning |
|-------------|-------|---------|
| First number (controller) | 0–119 | Which control you are changing (see table below) |
| Second number (value) | 0–127 | New level (0 = minimum, 127 = maximum) |

Example: **`control change 7 to 100`** sets **channel volume** (controller 7) to a high level on the channel you selected.

```blocks
let synth = midi.channel(1)
synth.controlChange(7, 100)   // volume up
synth.controlChange(64, 127)  // sustain pedal on (≥64 = pressed)
```

Common controller numbers (the receiving app decides how each is used):

| Controller | Typical use |
|------------|-------------|
| **1** | Modulation / vibrato |
| **7** | Channel volume |
| **10** | Pan (left–right) |
| **11** | Expression |
| **64** | Sustain pedal (0 = off, 127 = on) |
| **91** | Reverb amount |
| **93** | Chorus amount |

Controllers **120–127** are reserved for special **channel mode** messages; use the **`mode`** block for those (for example “all notes off”), not **control change**.

### Other useful MIDI blocks

| Block | What it does |
|-------|----------------|
| **note** / **note on** / **note off** | Play or stop musical notes |
| **set instrument** / **program change** | Change the instrument sound |
| **set pitch bend** | Bend pitch up or down |
| **play tone** / **play drum** | Shortcuts on channel 1 or the drum channel (10) |

The connected app must support the message you send. If nothing happens, try a different controller number or test with **note** blocks first to confirm the Bluetooth MIDI link is working.

## Supported Platforms

This package supports **iOS** (iPad/iPhone), **macOS** (Audio MIDI Setup), **Android** (nRF Connect, LightBlue), and **Windows** (with a BLE MIDI bridge app).
Any app that acts as a **BLE MIDI central** and supports the standard MIDI service can connect to the @boardname@.

### macOS Audio MIDI Setup

1. Open **Audio MIDI Setup** → **Window** → **Show MIDI Studio**.
2. Double-click the **Bluetooth** configuration icon.
3. In the **Central** section (bottom), wait for your @boardname@ to appear, select it, and click **Connect**.
4. The device should then show as a MIDI input in other apps.

**Important for Mac:** use **Project Settings → Bluetooth → No Pairing Required** for MIDI Studio (Just Works can pair silently but often still fails Core MIDI). Flash **v2.0.25+**.

You must flash **v2.0.21** or later on a **micro:bit v2**: v2.0.17 always advertises the **full 128-bit** MIDI UUID (`0x07` AD type). Older builds sometimes advertised only a 16-bit alias (visible in nRF Connect **after** connect, but invisible to macOS MIDI scan). (micro:bit v1 + macOS is not supported for discovery.)

### Apple GarageBand

* [iPhone/iPad App](https://itunes.apple.com/us/app/garageband/id408709785?mt=8)

Go to settings (gearwheel), click **Advanced**, click **Bluetooth MIDI device** and connect to the @boardname@.
If the @boardname@ is marked as offline, click **Edit** and **Forget** the device.

### Windows

The @boardname@ implements **standard BLE MIDI**, so Windows can use it, but Windows does **not** offer a built-in flow like macOS **MIDI Studio**. Pairing in **Settings → Bluetooth** alone usually does **not** create a MIDI port for your DAW.

**Recommended setup (Windows 11):**

1. Flash **v2.0.25+** on a **micro:bit v2** with **Project Settings → Bluetooth → No Pairing Required**.
2. Run the @boardname@ in **normal mode** (not the “PAIRING MODE” screen).
3. Install a **BLE MIDI bridge** that scans, connects, and exposes a virtual MIDI port, for example **[Perfect Bluetooth MIDI for Windows](https://github.com/mayerwin/Perfect-Bluetooth-MIDI-For-Windows)** (free, open source). Recent Windows 11 builds include [Windows MIDI Services](https://github.com/microsoft/MIDI); the bridge routes BLE MIDI into that stack until native BLE MIDI transport is available in-box.
4. In the bridge app: **Scan** → select **BBC micro:bit** → **Connect** → pick the virtual port in your DAW (Ableton, FL Studio, Reaper, Cubase, etc.).
5. If notes do not appear, use the bridge’s **channel detect** or mapping options; confirm **note** blocks work before testing **control change**.

**Other options on Windows:** any tool that implements a BLE MIDI **central** (some users test with phone apps; **nRF Connect for Desktop** needs a Nordic USB dongle and does not use the PC’s built-in Bluetooth).

**micro:bit v1 on Windows:** the extension still builds for v1, but BLE MIDI advertising fixes were developed for **v2**; use a v2 board for the most reliable Windows experience.

## micro:bit v2

micro:bit v2 uses the **CODAL** runtime, not the v1 **DAL/mbed** stack. Older releases of this extension only implemented BLE MIDI with mbed APIs, so they **did not compile for v2**. MakeCode then disabled the package for v2 boards and showed **[error 929](https://support.microbit.org/support/solutions/articles/19000121371-makecode-extension-compatibility-v1-and-v2)** (“extension not compatible with this board”).

From **v2.0.14** onward, the extension builds on both v1 and v2: v2 uses the same CODAL BLE service model as the built-in Bluetooth blocks. **v2.0.15+** advertises the MIDI service UUID; **v2.0.16** fixes connection timeouts; **v2.0.17+** fixes macOS MIDI scan; **v2.0.21** fixes advertising layout (UUID in scan response), open-mode connect, and empty read/notify handshake (read or CCCD, whichever comes first). If you still see error 929, remove and re-add the extension (or use this repo version) and download a fresh `.hex` after upgrading.

## Troubleshooting BLE scanners (Android / nRF Connect)

### The device does not show up when scanning

Pairing mode (**No Pairing Required** vs **Just Works**) is chosen in **Project Settings → Bluetooth**, not in this extension. For macOS MIDI Studio, use **No Pairing Required** and flash **v2.0.25+**.

If the board is missing from scans entirely, ensure the program is **running normally** (not the “PAIRING MODE” screen). **Pairing mode does not advertise the MIDI UUID**, so macOS MIDI Studio will not list the device until you **reset out of pairing mode** (press reset without A+B, or reflash).

Bluetooth must be enabled in the firmware: adding this extension pulls in the `bluetooth` package, which turns the stack on. You do **not** need separate “start accelerometer” blocks for MIDI to work.

### MIDI Studio shows the device but Connect reverts to Connect

1. Flash **v2.0.25+** (deferred handshake notify after Subscribe; disables Event service for faster GATT discovery).
2. Use **No Pairing Required** in Project Settings → Bluetooth (recommended for MIDI Studio on Mac).
3. **Reset bonds:** forget “BBC micro:bit” on the Mac; reflash or pairing mode (A+B + reset), then **reset without A+B** before scanning in MIDI Studio.
4. Connect only from **Audio MIDI Setup → Bluetooth → Central**, not System Settings.
5. In LightBlue on Mac: after **Subscribe**, confirm a **2-byte** notification (`80 80`) arrives — that is the handshake MIDI Studio needs.

### Yotta conflict on `microbit-dal.bluetooth.open`

If MakeCode reports a conflict between **bluetooth** and **bluetooth-midi** on `open`, upgrade to **bluetooth-midi v2.0.22+** (this repo), which no longer forces `open`/`whitelist` — only the main **Bluetooth** extension / Project Settings control pairing. Remove the bluetooth-midi “Just Works pairing” user config if it still appears; use **Project Settings → Bluetooth → Just Works** instead.

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

## Acknowledgements

**micro:bit v2** and **macOS MIDI Studio** compatibility (v2.0.14–v2.0.25), including advertising, GATT handshake, and MakeCode pairing settings, was developed with help from **[Cursor](https://cursor.com) Composer** (AI coding assistant in the Cursor IDE), in collaboration with [Robert Bilsland](https://github.com/RBilsland).

## Supported targets

* for PXT/microbit (micro:bit v1 and v2)
* for PXT/calliope

(The metadata above is needed for package search.)

## License

MIT

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
