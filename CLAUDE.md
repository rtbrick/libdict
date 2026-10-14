# CLAUDE.md

Guidance for Claude (and other contributors) working on **libdict**, a C11 library of
key-value data structures (AVL/hb, red-black, splay, weight-balanced, path-reduction,
treap, skip list, two hash tables) used by downstream applications such as RtBrick's
tooling. Stability and portability matter more than cleverness here.

## Repository layout

- `include/` – **public headers** (installed to `include/libdict/`). Everything here is API/ABI.
- `src/` – implementation; `dict_private.h`, `tree_common.*`, `hashtable_common.*` are private.
- `unit_tests.c` – CUnit test suite (single file, registered in `basic_tests[]`).
- `benchmark.c`, `demo.c`, `anagram.c` – tools, also useful as smoke tests.
- `CMakeLists.txt` (primary) and `GNUmakefile` (alternative) – keep **both** working.
- `vg` – helper to run a binary under valgrind memcheck.
- `.github/workflows/` – CI (test, package, release, pages).

## Build and test

```sh
# dependencies (Debian/Ubuntu)
sudo apt install build-essential cmake libcunit1-dev

# CMake (what CI uses)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure

# GNU make alternative
make && make test

# memory checking
./vg build/libdict-tests

# sanitizers (run before proposing any non-trivial change)
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all"
cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure

# slow tests (opt-in; e.g. the 6M-node wb_tree overflow test, ~2 s / ~400 MB)
LIBDICT_SLOW_TESTS=1 ./build/libdict-tests
```

If an ASan binary built with clang 14 segfaults inside `__asan::AsanInitInternal` before
any test runs, that is the known clash with high ASLR entropy on newer kernels, not a
libdict bug; run it under `setarch -R`.

The build uses `-Werror -Wall -Wextra -pedantic` (and `-Weverything` with clang in the
GNUmakefile). Code must compile warning-free with both **gcc and clang**. Never silence a
warning by weakening flags; fix the code. The only deliberate exceptions are in the
GNUmakefile's clang branch: `-Wno-declaration-after-statement` (meaningless for this C11
code) and, for the unit tests only, `-Wno-extra-semi-stmt` (CUnit's `CU_ASSERT` macros).

## Rule 1: every bug fix comes with a regression test

For every bug found or fixed:

1. **First write a unit test in `unit_tests.c` that reproduces the bug** and fails on the
   unfixed code. Confirm it fails, then apply the fix and confirm it passes.
2. Name it after the behavior, e.g. `test_rb_tree_remove_root_with_one_child`, declare it
   with the other prototypes, and register it in `basic_tests[]` (or a new suite if it
   belongs to a distinct area).
3. If the bug affects a shared code path (iterators, near-search `le/lt/ge/gt`, `select`,
   `clear`, `traverse`, `remove` during iteration), test it against **all** data structures
   that implement it, not only the one where it was reported.
4. Call `dict_verify()` after mutating operations in tests so structural invariants are
   checked, not just returned values.
5. Tests must be deterministic. If a structure uses randomness (skip list levels, treap
   priorities), seed it explicitly or supply a `dict_prio_func` so failures reproduce.
6. If a test is genuinely impossible (e.g. requires a specific allocator failure or a
   hardware property), say so in the commit message / PR and explain how the fix was
   verified instead. "Not possible" must be justified, not assumed.
7. `demo.c`, `anagram.c` and `benchmark.c` are programs, not library functions, so their
   fixes cannot get a unit test. Verify them with a scripted run under ASan + UBSan +
   LeakSanitizer that fails before and passes after, and put the command in the commit
   message.
8. Tests that are too slow or memory-hungry for every run return early unless
   `LIBDICT_SLOW_TESTS=1` is set, and print a note when skipped.

The commit message should reference the bug and the test that proves it.

## Rule 2: do not break applications that use libdict

Downstream code links against the shared library and compiles against the installed
headers. Assume any public symbol, type, macro or behavior is relied on by someone.

### API (source compatibility)

- Do **not** rename, remove, or change the signature of any function, typedef, macro or
  struct field in `include/*.h`.
- Do not change the semantics of existing functions: return values, ownership of
  keys/data, what `delete_func` receives, iterator validity rules (see "ITERATOR
  INVALIDATION" in `dict.h`), ordering guarantees, and the meaning of
  `dict_insert_result.inserted` / `dict_remove_result.removed`.
- New functionality is added as **new** functions/macros. Old ones may be marked
  deprecated in documentation but must keep working.
- The `dict_*` convenience macros (`dict_insert`, `dict_itor_next`, …) are part of the API.

### ABI (binary compatibility)

The macros in `dict.h` dereference `dict_vtable` and `itor_vtable` **inside the
application binary**, so their layouts are compiled into user code:

- Never reorder, remove, or change the type of members in `dict_vtable`, `itor_vtable`,
  `dict`, `struct dict_itor`, `dict_insert_result`, `dict_remove_result`.
- If a vtable must grow, append members **at the end only**, and make the corresponding
  macro tolerate a NULL entry (as `search_le`/`select` already do).
- Do not change sizes of public types (e.g. `unsigned` hash values to `uint64_t`/`size_t`)
  or the type of the global hooks `dict_malloc_func` / `dict_free_func`.
- Do not change enum values or `#define` constants that may be compiled into users.
- Keep exported symbol names stable; don't make an exported function `static` or `inline`.

### Versioning

- Any change that is not backward compatible is a design decision for the maintainers,
  not something to slip into a bug fix. Stop and ask before making one.
- If such a change is approved, bump the major version and SOVERSION together.
- Careful: `CMakeLists.txt` sets `SOVERSION` to `MAJOR.MINOR`, so even a **minor** version
  bump changes the soname and breaks binary compatibility for installed applications.
  Do not bump the minor version for a compatible release without changing that first.
- Note: versions are currently declared in several places (`project(... VERSION)` in
  `CMakeLists.txt`, `DICT_VERSION_*` in `include/dict.h`, `INSTALL_LIBVER` in
  `GNUmakefile`). They currently disagree (1.0.0, 0.3.0 and 2); keep them consistent when
  touching any of them, and keep `test_version_string` passing.
- Optional check before release: compare old vs new `libdict.so` with
  `abidiff` (libabigail) or `abi-compliance-checker`; any reported incompatibility blocks
  the release.

## Rule 3: portable across architectures and endianness

libdict must behave identically on **little- and big-endian** systems and on
**32- and 64-bit** targets: at least x86_64 (amd64), aarch64 (arm64), 32-bit ARM
(armhf), i386, and a big-endian target such as s390x or ppc64 (big-endian).

### Coding rules

- **No type punning** of byte buffers into integers (`*(uint32_t*)p`). Use `memcpy`
  or assemble values byte-by-byte. This avoids both endianness and alignment bugs
  (ARM and others may fault or be slow on unaligned access).
- Hash functions must operate on **bytes in a defined order** so a given key yields the
  same hash on every platform (as `dict_str_hash`'s FNV-1a does). Never hash the
  in-memory representation of a multi-byte integer without normalizing byte order.
- Do not assume `sizeof(long) == 8`, `sizeof(void*) == 8`, or `sizeof(int) == sizeof(void*)`.
  Use `size_t` for counts/sizes, `uintptr_t` for pointer-as-integer, fixed-width types
  (`uint32_t`, …) where width matters, and the correct `printf` format macros
  (`%zu`, `PRIu32`).
- **`char` signedness differs**: plain `char` is signed on x86 but unsigned on ARM.
  Use `unsigned char`/`uint8_t` when comparing or hashing bytes; compare strings via
  `strcmp`/`memcmp`, which are defined in terms of `unsigned char`.
- Avoid undefined behavior: signed overflow, shifts by ≥ width, `__builtin_ctz(0)` /
  `__builtin_clz(0)` (check for zero first, e.g. around `dict_rand()` in `skiplist.c`),
  out-of-range pointer arithmetic, and comparator functions that subtract values
  (`a - b`) instead of comparing them.
- Compiler builtins (`__builtin_expect`, `__builtin_ctz`, …) must have a portable
  fallback or be guarded, so the code still builds with non-GCC/Clang compilers.
- `random()` is POSIX, not ISO C; keep its use behind `dict_rand()` in `dict_private.h`.
- No inline assembly and no architecture-specific intrinsics without a generic fallback.

### Testing on other architectures

Run the test suite on at least one big-endian and one ARM target for changes touching
hashing, bit manipulation, memory layout, or random-level/priority code:

```sh
# example using Docker + QEMU binfmt (one-time: docker run --privileged --rm tonistiigi/binfmt --install all)
for arch in linux/arm64 linux/arm/v7 linux/386 linux/s390x; do
  docker run --rm --platform "$arch" -v "$PWD":/src:ro -w /src debian:trixie sh -c '
    apt-get update -qq && apt-get install -y -qq build-essential cmake libcunit1-dev >/dev/null &&
    cmake -S . -B /tmp/b -DCMAKE_BUILD_TYPE=Debug && cmake --build /tmp/b -j &&
    ctest --test-dir /tmp/b --output-on-failure'
done
```

Where a behavior should be platform-independent (e.g. `dict_str_hash` output), add a
unit test with **hard-coded expected values** so a mismatch on any architecture fails CI.

Suggested CI improvement (propose, don't silently add): extend `test.yml` with a matrix
job using `uraimo/run-on-arch-action` or QEMU for `aarch64`, `armv7`, `s390x`, plus a
native `ubuntu-24.04-arm` runner, and a sanitizer job.

## Style and general conduct

- Follow `.clang-format` and the existing code style; don't reformat unrelated code.
- Keep the code iterative rather than recursive (a stated design goal) and keep
  performance in mind; run `libdict-benchmark` before/after changes to hot paths and
  mention significant regressions.
- Public API changes need Doxygen comments in the header (docs are published from master).
- Keep changes minimal and focused: one bug, one test, one fix per commit where possible.
- Before finishing any task, verify: builds with gcc and clang, `ctest` passes,
  sanitizer build passes, no new warnings, public headers unchanged unless intended.
- When unsure whether a change affects compatibility or portability, **ask** rather
  than guess.