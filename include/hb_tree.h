/*
 * libdict -- height-balanced (AVL) tree interface.
 *
 * Copyright (c) 2001-2014, Farooq Mela
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBDICT_HB_TREE_H__
#define LIBDICT_HB_TREE_H__

#include "dict.h"

/** \file
 *
 * Height-balanced (AVL) tree.
 *
 * The heights of the two subtrees of every node differ by at most one, so
 * insert, search and remove take O(log n) time in the worst case. Selecting
 * the nth element takes O(n). The balance factor is stored in the low bits
 * of each node's parent pointer, so the allocator must return 4-byte aligned
 * memory. */

BEGIN_DECL

/** A height-balanced (AVL) tree. Opaque: create one with hb_tree_new(). */
typedef struct hb_tree hb_tree;

/** Create an empty tree that orders keys with |cmp_func| (must not be NULL).
 * Returns NULL if memory cannot be allocated. */
hb_tree*        hb_tree_new(dict_compare_func cmp_func);
/** Like hb_tree_new(), but wrapped in a generic ::dict for use with the
 * dict_* macros. Free it with dict_free(). Returns NULL if memory cannot be
 * allocated. */
dict*           hb_dict_new(dict_compare_func cmp_func);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL, and free the tree. Returns the number of elements
 * removed. Iterators over the tree may afterwards only be freed. */
size_t          hb_tree_free(hb_tree* tree, dict_delete_func delete_func);

/** Insert |key| unless an equal key is present. Returns {&datum, true} for a
 * new key, whose datum starts as NULL; the tree stores the |key| pointer
 * itself, it does not copy the key. If an equal key is present, returns
 * {&datum, false} with that key's datum, and |key| is not stored and still
 * belongs to the caller. Returns {NULL, false} if memory cannot be
 * allocated. */
dict_insert_result
                hb_tree_insert(hb_tree* tree, void* key);
/** Return the address of the datum stored for |key|, or NULL if |key| is not
 * present. */
void**          hb_tree_search(hb_tree* tree, const void* key);
/** Return the address of the datum of the greatest key less than or equal to
 * |key|, or NULL if there is none. */
void**          hb_tree_search_le(hb_tree* tree, const void* key);
/** Return the address of the datum of the greatest key less than |key|, or
 * NULL if there is none. */
void**          hb_tree_search_lt(hb_tree* tree, const void* key);
/** Return the address of the datum of the least key greater than or equal to
 * |key|, or NULL if there is none. */
void**          hb_tree_search_ge(hb_tree* tree, const void* key);
/** Return the address of the datum of the least key greater than |key|, or
 * NULL if there is none. */
void**          hb_tree_search_gt(hb_tree* tree, const void* key);
/** Remove the element with key |key|. Returns {key, datum, true}, handing the
 * stored key and datum back to the caller (no delete function is called), or
 * {NULL, NULL, false} if |key| is not present. See \ref
 * iterator_invalidation for which iterators this invalidates. */
dict_remove_result
                hb_tree_remove(hb_tree* tree, const void* key);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL. Returns the number of elements removed. The tree
 * stays usable. See \ref iterator_invalidation for which iterators this
 * invalidates. */
size_t          hb_tree_clear(hb_tree* tree, dict_delete_func delete_func);
/** Call |visit|(key, datum, |user_data|) for each element in key order,
 * stopping early if |visit| returns false. Returns the number of calls made. */
size_t          hb_tree_traverse(hb_tree* tree, dict_visit_func visit, void* user_data);
/** Store the key and datum of the |n|th element in key order (0-based)
 * through |key| and |datum| and return true, or store NULL through them and
 * return false if |n| is out of range. Either may be NULL. Takes O(n) time. */
bool            hb_tree_select(hb_tree* tree, size_t n, const void** key, void** datum);
/** Return the number of elements. Takes O(1) time. */
size_t          hb_tree_count(const hb_tree* tree);
/** Return the shallowest depth at which a child link is absent, the root
 * being at depth 1, or 0 for an empty tree. A statistic for testing and
 * benchmarking. */
size_t          hb_tree_min_path_length(const hb_tree* tree);
/** Return the greatest depth of any node, the root being at depth 1, or 0 for
 * an empty tree: the height of the tree. A statistic for testing and
 * benchmarking. */
size_t          hb_tree_max_path_length(const hb_tree* tree);
/** Return the sum of the depths of all nodes, the root being at depth 1, or 0
 * for an empty tree. A statistic for testing and benchmarking. */
size_t          hb_tree_total_path_length(const hb_tree* tree);
/** Check the internal invariants of the tree and return true if they hold, or
 * print the first failed check to stderr and return false. Meant for testing
 * and debugging; takes O(n) time. */
bool            hb_tree_verify(const hb_tree* tree);

/** An iterator over a height-balanced (avl) tree. Opaque: create one with
 * hb_itor_new(). */
typedef struct hb_itor hb_itor;

/** Create an iterator over |tree| that is not positioned on any element yet.
 * Returns NULL if memory cannot be allocated. */
hb_itor*        hb_itor_new(hb_tree* tree);
/** Like hb_itor_new(), but wrapped in a generic ::dict_itor for use with the
 * dict_itor_* macros. Free it with dict_itor_free(). */
dict_itor*      hb_dict_itor_new(hb_tree* tree);
/** Free an iterator. */
void            hb_itor_free(hb_itor* itor);

/** Return whether |itor| is positioned on an element. */
bool            hb_itor_valid(const hb_itor* itor);
/** Unposition |itor|, so that it is not positioned on any element. */
void            hb_itor_invalidate(hb_itor* itor);
/** Move |itor| to the next element in key order and return true, or
 * unposition it and return false if there is none. An unpositioned iterator
 * stays unpositioned and returns false. */
bool            hb_itor_next(hb_itor* itor);
/** Move |itor| to the previous element in key order and return true, or
 * unposition it and return false if there is none. An unpositioned iterator
 * stays unpositioned and returns false. */
bool            hb_itor_prev(hb_itor* itor);
/** Move |itor| |count| elements forward and return whether it is still
 * positioned. It becomes unpositioned if it runs past the last element. */
bool            hb_itor_nextn(hb_itor* itor, size_t count);
/** Move |itor| |count| elements back and return whether it is still
 * positioned. It becomes unpositioned if it runs past the first element. */
bool            hb_itor_prevn(hb_itor* itor, size_t count);
/** Position |itor| on the first element in key order and return true, or
 * unposition it and return false if the tree is empty. */
bool            hb_itor_first(hb_itor* itor);
/** Position |itor| on the last element in key order and return true, or
 * unposition it and return false if the tree is empty. */
bool            hb_itor_last(hb_itor* itor);
/** Position |itor| on |key| and return true, or unposition it and return
 * false if |key| is not present. */
bool            hb_itor_search(hb_itor* itor, const void* key);
/** Position |itor| on the greatest key less than or equal to |key| and return
 * true, or unposition it and return false if there is none. */
bool            hb_itor_search_le(hb_itor* itor, const void* key);
/** Position |itor| on the greatest key less than |key| and return true, or
 * unposition it and return false if there is none. */
bool            hb_itor_search_lt(hb_itor* itor, const void* key);
/** Position |itor| on the least key greater than or equal to |key| and return
 * true, or unposition it and return false if there is none. */
bool            hb_itor_search_ge(hb_itor* itor, const void* key);
/** Position |itor| on the least key greater than |key| and return true, or
 * unposition it and return false if there is none. */
bool            hb_itor_search_gt(hb_itor* itor, const void* key);
/** Return the key |itor| is positioned on, or NULL if it is not positioned.
 * The key belongs to the container and must not be modified in a way that
 * changes how it compares. */
const void*     hb_itor_key(const hb_itor* itor);
/** Return the address of the datum |itor| is positioned on, through which it
 * can be read or replaced, or NULL if it is not positioned. */
void**          hb_itor_datum(hb_itor* itor);
/** Compare the positions of two iterators over the same tree: return a
 * negative value, zero, or a positive value as |i1| is before, at, or after
 * |i2| in key order. An unpositioned iterator compares before every
 * positioned one. */
int             hb_itor_compare(const hb_itor* i1, const hb_itor* i2);
/** Remove the element |itor| is positioned on and unposition |itor|; return
 * false if it was not positioned. No delete function is called, so fetch the
 * key and datum first if they need freeing. See \ref iterator_invalidation
 * for which iterators this invalidates. */
bool            hb_itor_remove(hb_itor* itor);

END_DECL

#endif /* !LIBDICT_HB_TREE_H__ */
