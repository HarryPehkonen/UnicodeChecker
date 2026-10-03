# Examples — the checker on real bytes

Ten small files, each one demonstrating a part of the report. They exist so you can
open a file in an editor, run the tool over it, and see the finding for yourself
rather than take the README's word for it.

```sh
cmake -S . -B build && cmake --build build
./build/unicode_checker examples/05-tags-injection.txt

# or all of them at once
for f in examples/*; do echo "== $f"; ./build/unicode_checker "$f"; done
```

| File | What it demonstrates |
|---|---|
| `01-plain-ascii.txt` | The baseline: a clean file. Every section says `none` / `no`. |
| `02-bom-utf8.txt` | A UTF-8 BOM (`EF BB BF`) — reported once, by the BOM section. The invisible section stays silent, because U+FEFF at offset 0 *is* the BOM, not a finding. |
| `03-utf8-defects.txt` | One of every UTF-8 defect the decoder knows: an overlong pair (`C0 AF`), a lone continuation byte, an invalid start byte (`F8`), an invalid continuation, a surrogate (`ED A0 80`), a codepoint past U+10FFFF (`F4 90 80 80`), and a sequence truncated by the end of the file. Each counts 1. |
| `04-invisible.txt` | Five default-ignorable characters in ordinary prose: zero-width space, soft hyphen, right-to-left override, word joiner, and a U+FEFF *mid-file* (which is the one case where it is a finding). The RLO one is the filename trick: `report<U+202E>txt.exe` displays as `reportexe.txt`. |
| `05-tags-injection.txt` | The keyword-splitting case from the "invisible text" demonstrations: `fun` + U+E0061 + `ding` reads as **funding**, and the tool decodes the tag back to `a` and reports it at its byte offset. Line two hides a whole sentence in tags — the payload is reassembled in full. |
| `06-tags-flag-vs-hidden.txt` | The same five tag letters twice: first as `U+1F3F4` + `gbeng` + `U+E007F` (a valid England subdivision-flag emoji, counted separately and *not* as hidden text), then bare with no black flag (hidden text). This is the distinction that keeps the detector from crying wolf on ordinary flags. |
| `07-noncharacters.txt` | U+FDD0, U+FDEF and U+FFFF, two C1 controls, and a lone surrogate (`ED A0 80`). |
| `08-nul-interleaved.bin` | `H\0i\0 \0t\0h\0e\0r\0e\0` — NUL second in every pair, the UTF-16LE shape, flagged as interleaved and binary-like. |
| `09-nul-few.bin` | Two NULs in a long text file: counted, ratio reported, and **Binary-like: no**. NUL detection is a ratio, not a switch — 2 bytes in 788 is 0.25%, under the 0.5% threshold. |
| `10-everything-at-once.bin` | The kitchen sink: a BOM, a zero-width space, hidden tags, a noncharacter, invalid bytes and a NUL. Every section has something to say. |

## Seeing what the tool sees

Your editor renders most of this as nothing — that is the whole point of the files.
Vim can show you the bytes with what it already ships:

```sh
vim -b examples/05-tags-injection.txt    # -b: binary mode, no line-ending games
:%!xxd                                    # hex dump; :%!xxd -r to go back
```

The hidden `a` is `f3 a0 81 a1`, and every tag character starts with the same
`f3 a0 80`/`f3 a0 81` pair — the block's whole pattern. `xxd examples/06-tags-flag-vs-hidden.txt`
shows the black flag (`f0 9f 8f b4`) and the cancel tag (`f3 a0 81 bf`) around the letters.
Outside Vim: `xxd file | head`, `cat -v file`, or `sed -n l file`.

`08`, `09` and `10` contain NUL bytes, so treat them as binary files — `vim -b`, not
`vim`. `07` and `03` contain bytes that are not valid UTF-8 at all; the tool is
reporting on them rather than reading them as text, and so should you.

## These files are checked

`tests/examples_test.cpp` opens each file from this directory and asserts what it
demonstrates — the tag payloads, the flag classification, the NUL ratios, the
noncharacter kinds. An example that stops showing what the table above claims is a
test failure, not a stale doc. If you change one of these files, run the suite:

```sh
ctest --test-dir build --output-on-failure
```
