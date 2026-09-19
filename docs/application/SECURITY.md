# Security

English | [简体中文](SECURITY.zh_CN.md)

## Security goal

Generated passwords must come from a cryptographically suitable device random source and remain local unless the user explicitly selects **Send**. The firmware initializes BLE only as a HID keyboard; it does not initialize Wi-Fi or NFC, log passwords, keep password history, write passwords to NVS, or expose a clipboard or screen-capture path.

## Randomness design

ESP-IDF documents that `esp_random()`/`esp_fill_random()` provides true random numbers only while an entropy source such as RF or the bootloader random source is active. The AI Passport buttons use an ADC resistor ladder, so the platform adapter performs these steps before `bsp_button_init()` claims ADC1:

1. Create and initialize an Mbed TLS CTR-DRBG context.
2. Enable the ESP-IDF bootloader random entropy source.
3. Seed the DRBG through an `esp_fill_random()` entropy callback with a fixed personalization string.
4. Disable the bootloader entropy source before initializing the ADC button driver.
5. Serve generation requests from the mutex-protected DRBG.

The personalization string is public domain separation, not a secret or a seed. The implementation never uses `rand()`, a timestamp, `millis()`, a fixed seed, or an application LCG.

MoonBit receives randomness through a callback. Host tests replace it with `DeterministicSource`; that source and the host C stub are excluded from firmware generation.

## Unbiased mapping and policy

The MoonBit core uses rejection sampling before modulo reduction. Samples in the incomplete top bucket are discarded, so character and dictionary indexes are not made more likely by direct modulo bias.

Random passwords always include letters. When digits or symbols are enabled, one character from each enabled class is generated first and the result is Fisher–Yates shuffled. PIN output accepts only `0`–`9`. Passphrase indexes are checked against the dictionary count, and the generated C dictionary API checks every index and character offset.

## Secret lifetime

The C display buffer is 128 bytes, accepts printable ASCII only, and is erased with `mbedtls_platform_zeroize()` before a new generation, after a configuration change, and on output failure. The generator never prints its content to ESP logs.

BLE transfer copies the current result into one fixed-size queue item. The worker sends key-down/key-up HID reports and zeroizes both the queue item and report buffer when the attempt ends. Only BLE bonding keys and peer identity metadata may persist in NVS; generated passwords are never part of a bond record.

MoonBit's current native runtime does not expose a reliable explicit secure erase guarantee for all temporary objects. Random password generation uses a temporary `FixedArray`, and optimizer/runtime copies may survive until memory is reused. This is a documented limitation: the firmware reduces obvious retention but does not claim complete RAM forensic resistance.

LVGL also retains label text in its own object memory while the result is visible. A new result or configuration change replaces the visible value, but V1 does not claim that every internal LVGL allocation is immediately scrubbed.

## Failure behavior

If RNG initialization, mutex acquisition, DRBG generation, output bounds, or dictionary access fails, the adapter wipes the result and the UI shows the localized equivalent of “generation failed, retry.” It never substitutes a timestamp, predictable sequence, or placeholder that looks like a password.

## BLE keyboard boundary

- The device advertises as `FoloPassKey` and accepts one BLE connection.
- First pairing uses a six-digit passkey shown on the AI Passport. HID sending is disabled until the connection is encrypted and the host subscribes to the keyboard input report.
- A password is transmitted only after the user focuses **Send** and presses `OK`; generation never triggers automatic typing.
- Up to three BLE bonds may be retained so a known host can reconnect. Removing a host must be done from the host Bluetooth settings; a device-side bond-management screen is not included yet.
- HID usages follow a US keyboard layout. The host must use an English/direct-input layout for symbols to reproduce exactly; an IME or another keyboard layout can transform the received keystrokes.
- After typing, the destination application and operating system own the text. Browser form history, clipboard managers, accessibility tools, screen recording, or an incorrectly focused field can expose it outside the device's control.

## Not security claims

- The on-screen entropy value is an estimate based on the configured search space; it is not a password-strength audit against user-specific policies or leaked-password corpora.
- A successful host test or firmware build is not proof of physical RNG quality, display privacy, side-channel resistance, or device tamper resistance.
- The firmware provides one-way, user-confirmed BLE keyboard transfer, not password storage, recovery, synchronization, or management.

## Device acceptance still required

- Confirm repeated cold boots and generation do not report RNG failure.
- Confirm Random mode contains every enabled class and all output is readable.
- Confirm no generated value appears on the serial console.
- Confirm configuration changes replace the previous display value.
- Confirm Wi-Fi and NFC remain inactive and the system power-button behavior is unchanged.
- Pair `FoloPassKey` on Windows, Android, and iOS where available; verify the on-screen passkey, reconnect, disconnect/re-advertise, and bond persistence after reboot.
- With the host in English/direct-input mode, verify every generated character types exactly once into an intentionally focused non-sensitive test field and never appears in serial logs.
- Confirm sleep/wake and low-battery behavior on the target hardware.
