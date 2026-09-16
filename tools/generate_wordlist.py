#!/usr/bin/env python3
"""Convert the EFF short word list into a flash-resident C table."""

from __future__ import annotations

import argparse
from pathlib import Path
import re


EXPECTED_WORDS = 1296
WORD_RE = re.compile(r"^[a-z-]{3,5}$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    words: list[str] = []
    for line_number, line in enumerate(args.input.read_text(encoding="ascii").splitlines(), 1):
        fields = line.split("\t")
        if len(fields) != 2 or not WORD_RE.fullmatch(fields[1]):
            raise ValueError(f"invalid word-list line {line_number}: {line!r}")
        words.append(fields[1])
    if len(words) != EXPECTED_WORDS:
        raise ValueError(f"expected {EXPECTED_WORDS} words, found {len(words)}")

    offsets: list[int] = []
    offset = 0
    for word in words:
        offsets.append(offset)
        offset += len(word) + 1

    lines = [
        "/* Generated from assets/wordlists/eff-short-wordlist-1.txt. */",
        "#include <stdint.h>",
        "",
        f"#define PASSPORT_DICTIONARY_COUNT {len(words)}",
        "",
        "static const char s_dictionary_words[] =",
    ]
    lines.extend(f'    "{word}\\0"' for word in words)
    lines.append("    ;")
    lines.append("")
    lines.append("static const uint16_t s_dictionary_offsets[PASSPORT_DICTIONARY_COUNT] = {")
    for start in range(0, len(offsets), 16):
        chunk = ", ".join(str(value) for value in offsets[start:start + 16])
        lines.append(f"    {chunk},")
    lines.extend([
        "};",
        "",
        "int32_t passport_dictionary_count(void) {",
        "    return PASSPORT_DICTIONARY_COUNT;",
        "}",
        "",
        "int32_t passport_dictionary_length(int32_t index) {",
        "    if (index < 0 || index >= PASSPORT_DICTIONARY_COUNT) return -1;",
        "    const char *word = &s_dictionary_words[s_dictionary_offsets[index]];",
        "    int32_t length = 0;",
        "    while (word[length] != '\\0') length++;",
        "    return length;",
        "}",
        "",
        "int32_t passport_dictionary_char(int32_t index, int32_t offset_in_word) {",
        "    int32_t length = passport_dictionary_length(index);",
        "    if (offset_in_word < 0 || offset_in_word >= length) return -1;",
        "    return (unsigned char)s_dictionary_words[s_dictionary_offsets[index] + offset_in_word];",
        "}",
        "",
    ])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii", newline="\n")
    print(f"[wordlist-codegen] generated {args.output} ({len(words)} words)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
