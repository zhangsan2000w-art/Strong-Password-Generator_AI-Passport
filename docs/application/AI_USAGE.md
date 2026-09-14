# AI usage

English | [简体中文](AI_USAGE.zh_CN.md)

## Developer-owned direction

The developer defined the product scope, three password modes, parameter bounds and defaults, three-button interaction, offline/privacy requirements, MoonBit-first architecture, target hardware, build artifact, test expectations, documentation set, and the rule that build success must not be presented as device success.

## AI assistance

AI assistance was used to inspect the official repository and hardware contract, investigate the MoonBit native/C FFI path, implement MoonBit and C code, generate the compact word list and CJK font subset, write tests and documentation, and diagnose actual compiler and validation output.

AI-generated or AI-edited code was reviewed through source inspection and executable checks. Compiler errors were handled from their real diagnostics; successful results were not invented. The developer remains responsible for reviewing the code, explaining the design, running physical-device acceptance, and deciding whether to submit or publish a hackathon entry.

## Verification

- MoonBit check runs with warnings denied.
- Nine deterministic MoonBit tests cover generation policies, boundaries, character classes, passphrase formatting, dictionary indexes, rejection sampling, entropy bounds, and state transitions.
- ESP-IDF 5.5.3 compiles generated MoonBit C into the ESP32-C3 application.
- The merged image verifier checks bootloader, partition table, application offsets, partition fit, and the complete `-full.bin` image.
- Physical-device validation is intentionally reported as not run until a real AI Passport is flashed and observed.

## Human review checklist

Before hackathon submission, the developer should inspect the RNG initialization order and failure behavior, read the state-machine tests, confirm every UI string on hardware, verify the three-button flows, review third-party notices, and reproduce both validation commands from a clean checkout.
