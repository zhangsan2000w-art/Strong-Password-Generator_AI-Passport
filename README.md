# Strong Password Generator_AI Passport

English | [简体中文](README.zh_CN.md)

**Strong Password Generator_AI Passport** is an offline three-button password generator built for the FoloToy AI Passport and MoonBit Hackathon 2026.

The firmware starts directly in the generator. It does not connect to a network, store password history, or redefine the system power button. A generated value leaves the device only after the user explicitly selects **Send**, through the paired encrypted BLE HID keyboard connection.

## Features

- **Random**: 6–30 printable ASCII characters, default length 10, letters always enabled, optional digits and symbols. Every enabled optional class is guaranteed to appear.
- **Memorable**: 3–6 offline words, default 4, optional capitalization, complete or four-character abbreviated words, and `-`, `.`, or `_` separators.
- **PIN**: 4–12 decimal digits, default length 6.
- **Input**: `UP`, `DOWN`, and `OK` only. `OK` enters or confirms editing, toggles Boolean values, or generates. Holding `UP` / `DOWN` continuously changes a numeric value while editing. Long-pressing `OK` cancels an edit or opens Settings when the main screen is not editing.
- **Feedback**: MoonBit generates the success chime's frequencies, durations, envelope, and PCM samples; the C audio task only performs non-blocking playback. The chime can be disabled in Settings.
- **BLE keyboard**: pair the device named `FoloPassKey`, enter the six-digit passkey shown on the AI Passport, focus a field on the host, and select **Send** to type the current password as a US-layout keyboard.
- **Display**: switchable 240×320 cyberpunk and blue-sky themes with a 17px, 4bpp, strongly hinted CJK subset. MoonBit view models own parameter slots, focus, layout, theme and settings state, strength color, and battery presentation policy.
- **Core**: generation, unbiased indexes, output postconditions, entropy and strength, state transitions, settings input policy, view models, battery policy, and sound synthesis are implemented in MoonBit.

## Settings and persistence

- Long-press `OK` on the main screen while not editing to open Settings.
- Use `UP` / `DOWN` to select **Theme** or **Sound**, press `OK` to change the selected value, and long-press `OK` to return.
- **Theme** switches between the dark cyberpunk interface and the blue-sky, clouds, and grass interface.
- **Sound** enables or disables the success chime without affecting password generation.
- Theme and sound preferences and up to three BLE bond records are stored in ESP-IDF NVS. Generated passwords and password history are never persisted.
- If NVS is unavailable, the selected values still apply for the current session and the firmware logs a warning. The application does not erase the NVS partition automatically.

## MoonBit-first implementation

The repository now contains 2,804 production `.mbt` lines and 1,255 MoonBit test lines, 4,059 in total. Excluding tests, blank lines, and comments leaves 2,397 effective production MoonBit lines. `tools/check_repo.py` independently enforces at least 1,000 effective production lines; tests cannot satisfy that gate.

The production MoonBit modules are compiled into and called by the ESP-IDF firmware. They own:

- Random password, PIN, and passphrase algorithms plus enabled-class guarantees;
- the RandomSource abstraction, rejection sampling, and generated-output postconditions;
- parameter policies, entropy estimates, and strength classification;
- NAVIGATION / EDITING transitions and configuration change detection;
- settings focus, button-gesture mapping, theme selection, and sound policy;
- parameter slot, coordinate, focus, editing, and value view models;
- pure policy for resolving raw CW2017 readings into a display value;
- BLE keyboard state, send eligibility, and the complete printable-ASCII to USB HID report mapping;
- success-note sequencing, attack/release envelopes, and PCM sample generation.

C is restricted to ESP-IDF/BSP initialization, LVGL widget calls, raw I2C readings, FreeRTOS scheduling, the NVS persistence adapter, NimBLE HID transport, codec writes, the secure-random source, and Flash dictionary access.

## Upstream and attribution

This project is built on the open-source [FoloToy AI Passport](https://github.com/FoloToy/ai-passport) firmware and hardware support stack.

The upstream FoloToy AI Passport project is licensed under the MIT License.
Its original copyright and license notices are retained in this repository.

This repository adds the MoonBit-based password generator, product logic, interaction design, UI, tests, documentation, and related firmware modifications for MoonBit Hackathon 2026.

## Toolchains

- ESP-IDF 5.5.3, target `esp32c3`
- MoonBit with the native/C backend (`moon` and `moonc` on `PATH`)
- Python 3

The verified development snapshot used `moon 0.1.20260904` and `moonc v0.10.12+1634b282e`. The ESP-IDF build invokes [`tools/generate_moonbit.py`](tools/generate_moonbit.py), which compiles the MoonBit package to portable C and links it into the `moonbit_password` ESP-IDF component. Generated C is a build artifact and is not committed.

GitHub Actions installs MoonBit's `latest` stable channel because dated CLI bundles are not guaranteed to remain downloadable from the official CDN. The exact tool versions printed by each CI run are therefore part of that run's verification record; the version above remains the locally verified snapshot.

## Test

Run the full static and host-test gate:

```bash
./tools/validate.sh --static
```

Run the MoonBit core directly:

```bash
MOONBIT_NEW_NATIVE=0 moon -C moonbit check --target native --deny-warn
MOONBIT_NEW_NATIVE=0 moon -C moonbit test --target native --release
```

The deterministic test source is used only by host tests. Firmware randomness always crosses the C FFI boundary into the ESP32 adapter.

## Build

Activate ESP-IDF 5.5.3, ensure MoonBit is on `PATH`, then run:

```bash
./tools/validate.sh --firmware
```

To run every gate:

```bash
./tools/validate.sh
```

The firmware gate uses a fresh temporary build, runs `idf.py build`, creates a merged flash image with `idf.py merge-bin`, verifies its component offsets and partition bounds, and copies only the checked image to:

```text
build/FoloToy-AI-Passport-full.bin
```

`build/FoloToy-AI-Passport.bin` is only the application image and is not a substitute for the merged firmware.

On Windows, use an ESP-IDF PowerShell followed by Git Bash, or pass the active ESP-IDF Python explicitly if the Microsoft Store `python3` alias is unusable:

```bash
PYTHON='D:/path/to/idf-python/Scripts/python.exe' ./tools/validate.sh --static
```

If a constrained Windows environment cannot write to the configured ccache directory, set `IDF_NO_CCACHE=1` for the firmware gate. CI keeps ccache enabled by default.

## Flash

The merged image is written at offset `0x0`:

```bash
python -m esptool --chip esp32c3 --baud 460800 \
  --before default-reset --after hard-reset \
  write-flash 0x0 build/FoloToy-AI-Passport-full.bin
```

Alternatively, use the official AI Passport web flasher and select the same `-full.bin` file. Successful compilation or flashing is not evidence that the UI, buttons, fonts, power behavior, and RNG adapter have passed physical-device acceptance.

Flashing the merged image at `0x0` can reset the NVS region. After initial provisioning, use segmented `idf.py flash` during development when existing theme and sound preferences must be preserved.

## Offline word list and fonts

- The memorable mode bundles the 1,296-entry [EFF Short Wordlist for Passphrases #1](https://www.eff.org/files/2016/09/08/eff_short_wordlist_1.txt), attributed to the Electronic Frontier Foundation under CC BY 3.0 US. The tracked source SHA-256 is `8f5ca830b8bffb6fe39c9736c024a00a6a6411adb3f83a9be8bfeeb6e067ae69`.
- Build-time code generation packs all words into one NUL-separated constant byte blob with 16-bit offsets. The table remains in Flash and is not loaded wholesale into RAM at startup.
- The Chinese LVGL glyph subset was generated from Noto Sans SC. Its OFL 1.1 notice is tracked at [`assets/fonts/NotoSansSC-OFL.txt`](assets/fonts/NotoSansSC-OFL.txt). Only ASCII and V1 UI glyphs are compiled into the firmware; the current subset uses 17px, 4bpp, and strong autohinting for heavier small-screen strokes.
- The subset uses LVGL's compressed font format, so `sdkconfig.defaults` enables `CONFIG_LV_USE_FONT_COMPRESSED=y`. If panels and the generated password render but title, mode, and button labels are blank, rebuild from clean defaults and confirm this option is present in the generated `sdkconfig`.
- The vendored MoonBit runtime files retain their Apache-2.0 notice in [`components/moonbit_password/RUNTIME_LICENSE.txt`](components/moonbit_password/RUNTIME_LICENSE.txt). Project-authored code remains under the repository MIT license.

## Design and security

- [Architecture and decision record](docs/application/ARCHITECTURE.md)
- [Security model and limitations](docs/application/SECURITY.md)
- [AI usage disclosure](docs/application/AI_USAGE.md)
- [Official AI Passport documentation index](docs/README.md)

## Verification status

The current tree defines 69 MoonBit tests. MoonBit strict checking, the 2,397-line effective production gate, repository checks, 11 Python firmware-layout tests, and the ESP-IDF 5.5.3 firmware build and merged-image verification passed locally. The local native MoonBit test executable could not be built because the detected legacy Windows C compiler cannot find `stdint.h`; this is an environment failure, not a recorded test pass. BLE pairing and typing, the dual-theme Settings screen, preference persistence across reboot, sound toggle, fonts, buttons, battery behavior, and RNG adapter still require physical-device validation.
