# unicode_checker

A command-line tool that scans a file's bytes and reports interesting things
about them: byte order marks, UTF-8 well-formedness, invisible and bidi
control characters, noncharacters, and NUL patterns.

The tool is a **reporter, not a decider**. It lists evidence and never
declares which encoding a file "really" is.

## Build

```sh
cmake -S . -B build && cmake --build build
```

Requirements: a C++17 compiler (developed with g++ 14.2), CMake 3.16+, and
Google Test installed system-wide (`find_package(GTest REQUIRED)`).

## Test

```sh
cmake --build build --target unicode_checker_test && ./build/tests/unicode_checker_test
```

Every test uses in-memory `std::vector<uint8_t>` buffers, so no fixture files
are needed.

## Usage

```sh
unicode_checker <filepath>
unicode_checker --version        # or -V
```

Output goes to stdout. Exit status is `0` on success even when findings are
reported, `1` when the file cannot be read, and `2` on a usage error.

The version number has one source, `project(unicode_checker VERSION ...)` in
`CMakeLists.txt`; `cmake/version.hpp.in` is configured into the build tree as
`generated/version.hpp` so the binary can report it, and the gate's `version`
stage checks the two copies against each other and against the binary.

Example:

```
file: example.txt
size: 35 bytes

[BOM]
  UTF-8 BOM detected (EF BB BF)

[UTF-8]
  Valid UTF-8: no
  Overlong sequences: 1
  Surrogate codepoints: 1
  Codepoints > U+10FFFF: 0
  Invalid continuation bytes: 0
  Lone continuation bytes: 0
  Invalid start bytes: 0
  Truncated sequences: 0

[Invisible / Zero-Width / Bidi]
  U+200B Zero Width Space at byte 18
  U+00AD Soft Hyphen at byte 22

[Noncharacters / Controls]
  noncharacter U+FFFF at byte 24
  C1 control U+0093 at byte 27
  lone surrogate U+D800 at byte 29

[NUL / Binary]
  NUL bytes: 0 (0.00%)
  Interleaved NULs: none
  Leading NUL bytes: 0
  Binary-like: no
```

## Detectors

Each detector is a standalone unit with its own header, and each is tested on
its own.

### BOM — `src/detect_bom.*`

Compares the first bytes against the known BOM table: UTF-8 (`EF BB BF`),
UTF-16BE (`FE FF`), UTF-16LE (`FF FE`), UTF-32BE (`00 00 FE FF`), UTF-32LE
(`FF FE 00 00`), and UTF-7 (`2B 2F 76` followed by `38`, `39`, `2B` or `2F`).

Longer BOMs are tested first, because `FF FE 00 00` starts with the UTF-16LE
BOM: it is only reported as UTF-16LE when bytes 2 and 3 are not both zero. A
BOM only counts at offset 0; the same bytes anywhere else are plain data.

### UTF-8 — `src/detect_utf8.*`

A six-state machine (`Start`, `Tail1`, `Tail2`, `Tail3`, `Accept`, `Reject`)
driven by six byte classes: ASCII `00..7F`, continuation `80..BF`, and the
lead ranges `C0..DF`, `E0..EF`, `F0..F7`, plus `F8..FF`, which lead nothing.
The codepoint is assembled first and then range-checked, which is what lets
each defect be reported separately:

- **Overlong sequences** — a value below the minimum for its length, e.g.
  `C0 AF` encoding `/`.
- **Surrogate codepoints** — U+D800..U+DFFF encoded as UTF-8 (`ED A0 80`).
- **Codepoints > U+10FFFF** — e.g. `F4 90 80 80`, and every `F5..F7` lead,
  which cannot encode anything smaller.
- **Invalid continuation bytes** — a non-continuation byte inside a sequence.
  The offending byte is then re-examined as a lead byte, the usual
  resynchronisation rule.
- **Lone continuation bytes** — `80..BF` where a lead byte was expected.
- **Invalid start bytes** — `F8..FF`.
- **Truncated sequences** — input that ends mid-sequence.

The decoder emits a codepoint for every sequence whose shape decoded,
including the ill-formed ones, each tagged with its `CodepointKind`. That is
what lets the later detectors see what someone tried to smuggle through an
overlong or surrogate encoding.

### Invisible / zero-width / bidi — `src/detect_invisible.*`

Runs over the decoded codepoints and reports each occurrence with its byte
offset and character name: U+200B–U+200D (zero-width space, non-joiner,
joiner), U+200E/U+200F (LRM/RLM), U+202A–U+202E (bidi embeddings, overrides
and pop), U+2060 (word joiner), U+2061–U+2064 (invisible operators), U+00AD
(soft hyphen), and U+FEFF.

U+FEFF is only a finding when it is *not* at offset 0 — at offset 0 it is the
byte order mark, which the BOM detector already reports.

### Noncharacters / controls — `src/detect_nonchar.*`

Also runs over the decoded codepoints:

- **Noncharacters** — U+FDD0–U+FDEF, and U+FFFE/U+FFFF in every plane up to
  U+10FFFE/U+10FFFF.
- **Lone surrogates** — U+D800–U+DFFF.
- **C1 controls** — U+0080–U+009F.

### NUL / binary — `src/detect_nul.*`

Works on raw bytes rather than codepoints:

- Counts NUL bytes and their ratio to the file size.
- Flags the file as **binary-like** when more than 0.5% of it is NUL.
- Reports **interleaved NULs** when every byte pair is one NUL and one
  non-NUL throughout: NUL second is consistent with UTF-16LE, NUL first with
  UTF-16BE. At least two whole pairs are required as evidence.
- Counts **leading NUL bytes**.

## Gate

The repo is wired to the [AI-DEV-STARTER](https://github.com/HarryPehkonen/AI-DEV-STARTER)
kit (rung 5); `.ai-dev-starter.json` records the kit revision and the SHA-256 of
every copied artifact.

```sh
tools/ci.sh                     # the full tier
tools/ci.sh build tests         # the fast tier, what pre-commit runs
```

The two git hooks live in `.githooks/` and have to be armed once per clone —
git never copies hooks for you:

```sh
git config core.hooksPath .githooks
```

`INCIDENTS.md` is the log of real failures and the check that now catches each
one. Read it before deleting a check.

## Layout

```
CMakeLists.txt          C++17, builds the core library, CLI and tests
cmake/version.hpp.in    configured into the build tree as generated/version.hpp
src/main.cpp            CLI entry point
src/detect_bom.*        BOM table
src/detect_utf8.*       UTF-8 state machine and decoder
src/detect_invisible.*  zero-width / bidi scan
src/detect_nonchar.*    noncharacters, surrogates, C1 controls
src/detect_nul.*        NUL counting and UTF-16 interleave heuristics
src/report.*            Report struct, accumulator, plain-text formatter
tests/                  one Google Test file per detector
```

The detectors are compiled into a `unicode_checker_core` library that both
the `unicode_checker` binary and the `unicode_checker_test` binary link
against.
