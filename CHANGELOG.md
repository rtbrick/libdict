# Changelog

## 1.0.7 (2026-10-14)

Changes since 1.0.6. The public API and ABI are unchanged: existing code
compiles as before, and `abidiff` against 1.0.6 reports no removed or changed
functions. A few fixes do change behaviour that a program may have relied on;
they are listed first.

### Behaviour changes

- **String key order on x86.** `dict_str_cmp` now compares bytes as
  `unsigned char`, exactly as `strcmp()` does. On x86, x86_64 and i386, keys
  containing bytes >= 0x80 (for example UTF-8 text) therefore sort differently
  than before: `"é"` now sorts after `"a"`. This affects iteration order,
  `dict_select()` positions and the neighbours found by the near searches;
  lookups are unaffected. Nothing changes on ARM, PowerPC or s390x, which
  already behaved this way.
- **`hb_itor_search()` is an exact search.** It used to find the first key
  greater than or equal to the target. `dict_itor_search()` on an hb_tree was
  already exact; use `hb_itor_search_ge()` for the old behaviour.
- **Hash table iterators detect invalidation.** After an insert that grows the
  table, an explicit resize, or `dict_clear()` (and, for `hashtable2`, after
  any removal), every iterator over the table reports itself invalid:
  `dict_itor_valid()` is false and `dict_itor_next()` returns false. Loops
  that modify a hash table while iterating now stop instead of silently
  skipping or repeating elements, or reading freed memory. Reposition with
  `dict_itor_first()` or `dict_itor_search()`. Inserting another key without a
  resize, and (for `hashtable`) removing another element, keep iterators
  valid as before. The rules for all containers are documented under
  "Iterator invalidation".
- **Inserting an existing key never grows a hash table.** Previously the
  resize check ran before the lookup.
- **vtable slots hold internal wrapper functions.** Calling container
  functions through incompatible function pointer types was undefined
  behaviour. As a result, comparing a vtable slot with a public function (for
  example `dct->_vtable->insert == hashtable_insert`) is now always false;
  compare the `_vtable` pointer with that of a dict of the known type instead.

### Fixed

- `dict_traverse()` on an hb_tree with two or more keys crashed.
- `dict_select()` with a NULL key or datum argument crashed on sp_tree and
  tr_tree.
- `skiplist_new()` with `max_link` 0 or 1 lost every element and leaked it;
  `max_link` is now raised to 2.
- wb_tree rebalancing broke above about 6 million elements (32-bit overflow).
- A degenerate splay tree or treap (about 175,000+ nodes in a chain) crashed
  the `*_path_length` functions and `sp_tree_verify`/`tr_tree_verify` with a
  stack overflow; they are now iterative.
- On 32-bit targets, creating or resizing a hash table with about 2^30 slots
  or more allocated too little memory and wrote past it; it now fails.
- hb_tree and rb_tree now abort with a message, also in release builds, if
  the allocator returns misaligned memory, instead of corrupting the tree.
- Undefined behaviour: `__builtin_ctz(0)` in the skip list, relational
  comparison of unrelated pointers in `dict_ptr_cmp`, and comparisons of
  pointers to freed nodes in several trees.
- `dict_verify()` now catches corruption it used to accept: wrong black
  height in rb_tree, a subtree on the wrong side of an ancestor (all trees), a
  wrong element count, and out-of-order skip list links. It prints the failed
  check to stderr.
- `dict_itor_compare()` on a hash table evaluates to 0 instead of calling a
  NULL function pointer; `hashtable2_itor_valid()` no longer aborts after a
  removal.
- sp_tree now splays when inserting an existing key, preserving its amortized
  O(log n) bound; the `rotation_count` statistics of sp_tree and pr_tree
  count single and double rotations consistently.
- `demo`, `anagram` and `benchmark`: crashes on a bare `search` command and
  on blank or NUL-led input lines, an overlapping `strcpy`, negative `<ctype.h>`
  arguments, `printf` format mismatches, unchecked allocations, and memory
  leaks. `benchmark` now skips blank input lines; `anagram` skips words with
  ten or more of one letter.
- `util.h` could not be included by two translation units.

### Documentation

- Every function of the public headers is documented, and the published
  Doxygen documentation now includes it (header comments were not extracted
  before) with the README as its main page.
- The README described `dict_insert_result.inserted` backwards; it is `true`
  when the key was inserted.
- New: iterator invalidation rules for every container, the comparison
  function contract, treap priority direction (max-heap), and usage notes in
  the README. Many wrong comments were corrected.

### Build and tests

- The GNU makefile works again (its recipe tabs had been replaced by spaces)
  and builds warning-free with clang `-Weverything`.
- The test suite grew from 13 to 41 tests, including checks of every
  container for traversal, selection and iterator behaviour. Slow tests (a
  6-million-element tree) run only with `LIBDICT_SLOW_TESTS=1`.

## 1.0.6 and earlier

See the [GitHub releases](https://github.com/rtbrick/libdict/releases) and the
git history.
