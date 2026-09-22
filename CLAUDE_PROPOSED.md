# unicode_checker — the contract for whoever (or whatever) works in this repo

## What this is

A single-file scanner that reports what a file's bytes are made of: byte order marks, UTF-8
well-formedness, invisible / zero-width / bidi characters, the Plane 14 Unicode Tags block,
noncharacters and C1 controls, and NUL / binary heuristics. It is a **reporter, not a decider** —
it lists evidence per section and never declares which encoding a file "really" is. "Working"
means: one `unicode_checker <file>` run prints every section, and the gate prints `GATE PASSED`.

## Commands

| What | Command |
|---|---|
| Configure + build | `cmake -S . -B build && cmake --build build -j"$(nproc)"` |
| Run the gate (do this before you claim anything works) | `tools/ci.sh` |
| Fast tier — what a commit runs | `tools/ci.sh build tests` |
| Tests only | `ctest --test-dir build --output-on-failure` |
| Format the files you touched | `clang-format -i <files>` |
| Run it locally | `./build/unicode_checker <file>` |
| Version | `./build/unicode_checker --version` |

The gate's last line is the verdict: `GATE PASSED` or `GATE FAILED`. Never report work as done on a
run that did not print `GATE PASSED`. A stage that SKIPs because a tool is missing on this machine
has not passed. `.githooks/` is armed per clone (`git config core.hooksPath .githooks`) — git never
copies hooks for you, and a fresh clone that skips that line is ungated until it is run again.

## Architecture

One decoder feeds every detector, and one struct collects their findings.

- `src/detect_utf8.cpp` — the only UTF-8 state machine. It decodes the buffer into
  `DecodedCodepoint{value, offset, length, kind}` and counts each class of defect. **Every other
  detector works on those codepoints**, never on raw bytes, and it never throws.
- `src/detect_bom.cpp`, `src/detect_nul.cpp` — byte-level (a BOM is bytes, not a codepoint).
- `src/detect_invisible.cpp`, `src/detect_nonchar.cpp`, `src/detect_tags.cpp` — codepoint-level,
  each `detect_x(const std::vector<DecodedCodepoint>&)` with a convenience byte overload.
- `src/report.cpp` — `analyze()` runs them in order into `Report`; `format_report()` prints one
  section per detector, one finding per line. `src/main.cpp` is only file I/O plus the call.

The seam to change when adding a detector: a new `detect_x.{h,cpp}`, a field on `Report`, one line
in `analyze()`, one section in `format_report()`, and the file in `CMakeLists.txt`.

## Invariants

- The version number has two copies that must agree: `project(unicode_checker VERSION x.y.z)` in
  `CMakeLists.txt` and the generated `build/generated/version.hpp` (from `cmake/version.hpp.in`) —
  enforced by the gate's `version` stage, and `unicode_checker --version` prints it.
- Every test file must be listed in `tests/CMakeLists.txt`; a test file that is not there is
  silently never run, and no stage can see the omission.
- `tests/CMakeLists.txt` registers the suite with `add_test()`, so the gate's default test command
  (ctest) is the whole suite.
- Every copied kit artifact's `repo_sha256` in `.ai-dev-starter.json` is updated in the same commit
  that edits it.

## Conventions

- C++17, no third-party dependency beyond system Google Test. Do not add a Unicode library: the
  repo works on codepoints it decodes itself.
- 4-space indent, 100 columns, `.clang-format` is the authority; the gate checks the files you touch.
- Tests live in `tests/*_test.cpp` with Google Test, over in-memory `std::vector<std::uint8_t>`
  buffers — no fixture files.
- Commits: `type(scope): summary` (`feat`, `test`, `fix`, `process`, `format`, `docs`), one logical
  change each.
- Offsets are the byte offset of a sequence's lead byte; ranges are half-open.

## Gotchas

- **A test-first change is ONE commit, not two.** The gate *is* the pre-commit hook, so a commit
  whose tests fail is refused — correctly. Write the tests plus a **compiling stub** of the
  interface, build and run them, keep the RED output, then implement and land RED+GREEN together
  with the RED evidence in the message. `--no-verify` is never the answer.
- **`project(<name> VERSION <x.y.z> CXX)` does not configure.** CMake 3.31 rejects a bare language
  argument after VERSION/DESCRIPTION/HOMEPAGE_URL: it must read
  `project(unicode_checker VERSION 0.1.0 LANGUAGES CXX)`. The gate's regex accepts the
  `LANGUAGES` form too, so the accepted form is also the form the version stage reads.
- **A codepoint in Plane 14 is not necessarily a Tag character.** The Tags block is
  **U+E0000–U+E007F**; U+E0100–U+E01EF is the variation-selector range and U+E0080–U+E00FF is
  unassigned. A flag emoji (`U+1F3F4` + tag letters + `U+E007F`) is legitimate use of the block,
  not evasion — the detector counts it separately.
- **A `build/` directory is machine-specific and never committed.** One carried over from another
  host or path makes `cmake -S . -B build` fail with "CMakeCache.txt is different than the
  directory … where it was created", and the cached binary then fails with "Exec format error".
  Delete it and let the gate make a fresh one.
- `char` is signed on x86: compare `std::uint8_t`, not `char`, when classifying bytes. The decoder
  does; new probe code that does not will look right and misbehave above 0x7F.

## Do not

- Do not add stripping, sanitizing or rewriting. The tool reports; the caller decides. No `--strip`,
  no removal API.
- Do not add a second UTF-8 parser, or a byte-pattern fast path that bypasses the decoder. The Tags
  block's 4-byte pattern (`F3 A0 (80|81) 80..BF`) is worth asserting in a test, not worth parsing
  twice.
- Do not edit generated files: `build/generated/version.hpp`. Change `cmake/version.hpp.in`.
- Do not print raw control bytes into the report — render them readably (`<U+E0001>`, `<CANCEL TAG>`).
- Do not edit `tools/ci.sh` while a run of it is in flight, and never "fix" the gate by deleting a
  check. Every check carries its incident in `INCIDENTS.md`; argue in a comment instead.

## Incidents

Every rule above that came from a failure is written down in `INCIDENTS.md` with the symptom, the
check that now catches it, and why deleting the check re-enables the bug. Read it before removing
anything that looks redundant.
