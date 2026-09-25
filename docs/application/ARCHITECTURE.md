# Architecture and decision record

English | [简体中文](ARCHITECTURE.zh_CN.md)

## Boundaries

```text
UP / DOWN / OK
      |
AI Passport BSP button callback -- queue only
      |
FreeRTOS input task + LVGL/hardware adapter (C)
      |
MoonBit product core: generation, state, view, HID mapping, battery, strength, sound
      |
C FFI: raw hardware input and output
      |
ESP-IDF + official AI Passport BSP
```

MoonBit owns the product rules. C owns the platform boundary. No MoonBit core module imports LVGL, GPIO, FreeRTOS, or an AI Passport header.

## MoonBit core

| Module | Responsibility |
| --- | --- |
| `policy.mbt` | Length bounds, character sets, separators |
| `random_source.mbt` | Callback-based random abstraction and rejection sampling |
| `password.mbt` | Random password and PIN generation |
| `passphrase.mbt` | Dictionary selection and formatting |
| `security_policy.mbt` | ASCII classification and generated-output postconditions |
| `entropy.mbt` | Display-oriented entropy estimates |
| `strength.mbt` | Configuration validity and strength classification |
| `state.mbt` | Packed application model and NAVIGATION/EDITING transitions |
| `view_model.mbt` | Parameter slots, layout, focus, editing state, and values |
| `battery.mbt` | CW2017 SOC/voltage trust and display policy |
| `sound.mbt` | Success-note sequence, envelope, and PCM samples |
| `ble_keyboard.mbt` | Send eligibility, transport-state presentation, and printable ASCII to USB HID mapping |
| `ffi.mbt` | Stable exported C API and platform callbacks |

The state is packed into a `UInt64` so the C adapter treats it as an opaque value. State transitions clear the one-shot action field before processing each input. A generation action can therefore be repeated intentionally by pressing `OK` again on the generate row.

There are 1,630 production MoonBit lines, or 1,223 effective lines after excluding tests, blanks, and comments. The 659 MoonBit test lines are tracked separately and cannot satisfy the production threshold. Repository checks prevent effective production MoonBit from dropping below 1,000 lines.

## Platform integration

`tools/generate_moonbit.py` invokes `moonc build-package` and `moonc link-core` for the native target. ESP-IDF compiles the resulting C with the vendored MoonBit runtime. Test files and host stubs are excluded from firmware code generation.

The FFI surface contains only integer values, an opaque 64-bit state, one `UInt` random callback, character output, and dictionary lookup. View, battery, and sound functions also export primitive integers, keeping LVGL, I2C, and ES8311 types outside MoonBit. The application can move to another board by replacing hardware calls without rewriting product policy.

The EFF dictionary is generated at build time into one constant NUL-separated byte array plus a `uint16_t` offset table. Individual characters are exposed to MoonBit on demand. No heap-sized dictionary copy is created at startup.

## UI and concurrency

The 240×320 LVGL screen is created directly at boot. All changing labels use printable ASCII or the bundled 17px, 4bpp, strongly hinted Noto Sans SC subset. The UI uses a dark-blue, cyan, magenta, and lime cyberpunk palette; normal focus and EDITING have distinct highlights and borders.

The BSP button callback copies `{button,event}` into a fixed-depth FreeRTOS queue and returns. The input task performs the MoonBit transition or generation, then takes the BSP LVGL lock only for the visual refresh. Parameter visibility, coordinates, focus, and strength classification come from the MoonBit view model. A successful generation only notifies a low-priority audio worker; MoonBit generates every sample of the approximately 160ms chime while C only initializes ES8311 and writes PCM.

`password_ble_keyboard.c` owns the NimBLE/HID transport. Its GAP callbacks publish transport states while NimBLE performs bonded, encrypted Just Works pairing without a passkey. An explicit MoonBit send action snapshots the current display buffer into a depth-one FreeRTOS queue; a dedicated worker converts each character through the MoonBit HID map, sends key-down/key-up reports, and zeroizes the transient buffers. The LVGL timer reads status only and never sends or logs a password.

## Decisions

| Decision | Why | Main alternative | Verification | Current limit |
| --- | --- | --- | --- | --- |
| MoonBit native-to-C code generation | Places the real product core in the firmware while preserving ESP-IDF tooling | Reimplement core in C | Build output compiles `generated/password_core.c`; exported symbols link into the app | Requires a compatible MoonBit toolchain during build |
| Effective production-MoonBit gate | Prevents the product layer regressing into a thin wrapper below the entry requirement and excludes test padding | Show only GitHub language share or count manually | `check_repo.py` excludes tests, blanks, and comments and requires at least 1,000 lines | Line count is only a floor; responsibility and tests still matter |
| Callback-based random source | Keeps algorithms independent of ESP-IDF and supports deterministic tests | Call `esp_random()` inside core | Rejection and deterministic-sequence tests | C callback failure is represented through output failure state |
| CTR-DRBG seeded before button ADC initialization | Hardware entropy is available before GPIO0 ADC ladder ownership and subsequent requests do not compete for ADC1 | Keep RF active for every random call | ESP-IDF build plus on-device generation acceptance | Seed health and long-duration behavior require hardware testing |
| Rejection sampling | Avoids modulo bias for character, digit, and word indexes | Direct modulo | Boundary test forces rejection of the incomplete bucket | Statistical certification is outside V1 |
| Flash-packed 1,296-word list | Bounded Flash and near-zero startup RAM overhead | Parse/load a text file at boot | Generator validates count and word format; firmware layout check | Vocabulary is intentionally smaller than EFF's long list |
| 17px, 4bpp strongly hinted CJK subset | Prevents missing glyphs and thin strokes without a full font | English-only UI, full CJK, or the earlier 16px/2bpp subset | All V1 strings are included and host builds can consume it | Weight still needs target-screen confirmation; new text requires regeneration |
| No settings persistence in V1 | Avoids accidental password persistence and keeps V1 focused | Persist non-secret preferences in NVS | Source audit and device acceptance | Preferences reset on reboot |
| CW2017 built-in profile without boot reset | The same-device reference is stable; this app showed cold-start 0% and later 77% after issuing `0x30 → 0x00` | Custom profile; reset gauge every boot; voltage only | Pure logic test covers implausible 0% fallback; device retest remains | A full charge/discharge check is still required |
| MoonBit synthesis with C asynchronous playback | Keeps product feedback policy in MoonBit without an audio asset or Flash cost | C synthesis, synchronous playback, or WAV | MoonBit waveform/envelope tests and firmware build | Volume and perceived sound need hardware confirmation |
| Encrypted Just Works BLE HID with explicit Send | Removes passkey entry while keeping bonded encryption and user-confirmed transfer | Passkey pairing, clipboard sync, BLE UART, QR code, or automatic typing | MoonBit map/state tests, ESP-IDF build, then cross-host device acceptance | Just Works has no MITM authentication; host focus, IME/layout, bond removal, and end-to-end typing require hardware testing |
