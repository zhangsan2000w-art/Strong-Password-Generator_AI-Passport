# Architecture and decision record

English | [简体中文](ARCHITECTURE.zh_CN.md)

## Boundaries

```text
UP / DOWN / OK
      |
AI Passport BSP button callback -- queue only
      |
FreeRTOS input task + LVGL adapter (C)
      |
MoonBit state model and password core
      |
C FFI: secure random, output buffer, Flash dictionary
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
| `entropy.mbt` | Display-oriented entropy estimates |
| `state.mbt` | Packed application model and NAVIGATION/EDITING transitions |
| `ffi.mbt` | Stable exported C API and platform callbacks |

The state is packed into a `UInt64` so the C adapter treats it as an opaque value. State transitions clear the one-shot action field before processing each input. A generation action can therefore be repeated intentionally by pressing `OK` again on the generate row.

## Platform integration

`tools/generate_moonbit.py` invokes `moonc build-package` and `moonc link-core` for the native target. ESP-IDF compiles the resulting C with the vendored MoonBit runtime. Test files and host stubs are excluded from firmware code generation.

The FFI surface contains only integer values, an opaque 64-bit state, one `UInt` random callback, character output, and dictionary lookup. The application can move to another board by replacing these callbacks and the UI adapter without rewriting generation rules.

The EFF dictionary is generated at build time into one constant NUL-separated byte array plus a `uint16_t` offset table. Individual characters are exposed to MoonBit on demand. No heap-sized dictionary copy is created at startup.

## UI and concurrency

The 240×320 LVGL screen is created directly at boot. All changing labels use either printable ASCII or the bundled Noto Sans SC subset. Focus uses a filled background and border; EDITING uses a separate orange color.

The BSP button callback copies `{button,event}` into a fixed-depth FreeRTOS queue and returns. The input task performs the MoonBit transition or generation, then takes the BSP LVGL lock only for the visual refresh. RNG and generation never run inside the callback or while holding the LVGL lock.

## Decisions

| Decision | Why | Main alternative | Verification | Current limit |
| --- | --- | --- | --- | --- |
| MoonBit native-to-C code generation | Places the real product core in the firmware while preserving ESP-IDF tooling | Reimplement core in C | Build output compiles `generated/password_core.c`; exported symbols link into the app | Requires a compatible MoonBit toolchain during build |
| Callback-based random source | Keeps algorithms independent of ESP-IDF and supports deterministic tests | Call `esp_random()` inside core | Rejection and deterministic-sequence tests | C callback failure is represented through output failure state |
| CTR-DRBG seeded before button ADC initialization | Hardware entropy is available before GPIO0 ADC ladder ownership and subsequent requests do not compete for ADC1 | Keep RF active for every random call | ESP-IDF build plus on-device generation acceptance | Seed health and long-duration behavior require hardware testing |
| Rejection sampling | Avoids modulo bias for character, digit, and word indexes | Direct modulo | Boundary test forces rejection of the incomplete bucket | Statistical certification is outside V1 |
| Flash-packed 1,296-word list | Bounded Flash and near-zero startup RAM overhead | Parse/load a text file at boot | Generator validates count and word format; firmware layout check | Vocabulary is intentionally smaller than EFF's long list |
| Generated CJK subset | Prevents missing Chinese glyphs without a full font | English-only UI or full CJK font | All V1 strings are included in generation range; physical screen remains unverified | New UI text requires regenerating the subset |
| No settings persistence in V1 | Avoids accidental password persistence and keeps V1 focused | Persist non-secret preferences in NVS | Source audit and device acceptance | Preferences reset on reboot |
