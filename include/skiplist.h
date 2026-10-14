/*
 * libdict -- skiplist interface.
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

#ifndef LIBDICT_SKIPLIST_H__
#define LIBDICT_SKIPLIST_H__

#include "dict.h"

/** \file
 *
 * Skip list [Pugh 1990].
 *
 * A sorted linked list with additional express links on randomly chosen
 * levels, so insert, search and remove take expected O(log n) time. It
 * supports the near searches but not selecting the nth element. Levels come
 * from random(), which libdict does not seed. */

BEGIN_DECL

/** A skip list. Opaque: create one with skiplist_new(). */
typedef struct skiplist skiplist;

/** Create an empty skip list that orders keys with |cmp_func| (must not be
 * NULL). |max_link| is the number of levels in the list. It is clamped into
 * the usable range [2, 32]: a node receives at most max_link - 1 links, so
 * fewer than 2 levels could not link anything. Returns NULL if memory cannot
 * be allocated. */
skiplist*       skiplist_new(dict_compare_func cmp_func, unsigned max_link);
/** Like skiplist_new(), but wrapped in a generic ::dict for use with the
 * dict_* macros. Free it with dict_free(). Returns NULL if memory cannot be
 * allocated. */
dict*           skiplist_dict_new(dict_compare_func cmp_func, unsigned max_link);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL, and free the list. Returns the number of elements
 * removed. Iterators over the list may afterwards only be freed. */
size_t          skiplist_free(skiplist* list, dict_delete_func delete_func);

/** Insert |key| unless an equal key is present. Returns {&datum, true} for a
 * new key, whose datum starts as NULL; the list stores the |key| pointer
 * itself, it does not copy the key. If an equal key is present, returns
 * {&datum, false} with that key's datum, and |key| is not stored and still
 * belongs to the caller. Returns {NULL, false} if memory cannot be
 * allocated. */
dict_insert_result
                skiplist_insert(skiplist* list, void* key);
/** Return the address of the datum stored for |key|, or NULL if |key| is not
 * present. */
void**          skiplist_search(skiplist* list, const void* key);
/** Return the address of the datum of the greatest key less than or equal to
 * |key|, or NULL if there is none. */
void**          skiplist_search_le(skiplist* list, const void* key);
/** Return the address of the datum of the greatest key less than |key|, or
 * NULL if there is none. */
void**          skiplist_search_lt(skiplist* list, const void* key);
/** Return the address of the datum of the least key greater than or equal to
 * |key|, or NULL if there is none. */
void**          skiplist_search_ge(skiplist* list, const void* key);
/** Return the address of the datum of the least key greater than |key|, or
 * NULL if there is none. */
void**          skiplist_search_gt(skiplist* list, const void* key);

/** Remove the element with key |key|. Returns {key, datum, true}, handing the
 * stored key and datum back to the caller (no delete function is called), or
 * {NULL, NULL, false} if |key| is not present. See \ref
 * iterator_invalidation for which iterators this invalidates. */
dict_remove_result
                skiplist_remove(skiplist* list, const void* key);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL. Returns the number of elements removed. The list
 * stays usable. See \ref iterator_invalidation for which iterators this
 * invalidates. */
size_t          skiplist_clear(skiplist* list, dict_delete_func delete_func);
/** Call |visit|(key, datum, |user_data|) for each element in key order,
 * stopping early if |visit| returns false. Returns the number of calls made. */
size_t          skiplist_traverse(skiplist* list, dict_visit_func visit, void* user_data);
/** Return the number of elements. Takes O(1) time. */
size_t          skiplist_count(const skiplist* list);
/** Check the internal invariants of the list and return true if they hold, or
 * print the first failed check to stderr and return false. Meant for testing
 * and debugging; takes O(n) time. */
bool            skiplist_verify(const skiplist* list);

/** Compute the histogram of link counts of the skiplist.
 * For 0 <= x < |ncounts|, |counts|[x] will be set to the number of nodes with x
 * links, and the maximal link count will be returned. If the return value is
 * greater than or equal to |ncounts|, not all link counts could be stored in
 * |counts| (i.e. the array was not large enough). */
size_t          skiplist_link_count_histogram(const skiplist* list,
                          size_t counts[], size_t ncounts);

/** An iterator over a skip list. Opaque: create one with
 * skiplist_itor_new(). */
typedef struct skiplist_itor skiplist_itor;

/** Create an iterator over |list| that is not positioned on any element yet.
 * Returns NULL if memory cannot be allocated. */
skiplist_itor*  skiplist_itor_new(skiplist* list);
/** Like skiplist_itor_new(), but wrapped in a generic ::dict_itor for use
 * with the dict_itor_* macros. Free it with dict_itor_free(). */
dict_itor*      skiplist_dict_itor_new(skiplist* list);
/** Free an iterator. */
void            skiplist_itor_free(skiplist_itor* itor);

/** Return whether |itor| is positioned on an element. */
bool            skiplist_itor_valid(const skiplist_itor* itor);
/** Unposition |itor|, so that it is not positioned on any element. */
void            skiplist_itor_invalidate(skiplist_itor* itor);
/** Move |itor| to the next element in key order and return true, or
 * unposition it and return false if there is none. An unpositioned iterator
 * stays unpositioned and returns false. */
bool            skiplist_itor_next(skiplist_itor* itor);
/** Move |itor| to the previous element in key order and return true, or
 * unposition it and return false if there is none. An unpositioned iterator
 * stays unpositioned and returns false. */
bool            skiplist_itor_prev(skiplist_itor* itor);
/** Move |itor| |count| elements forward and return whether it is still
 * positioned. It becomes unpositioned if it runs past the last element. */
bool            skiplist_itor_nextn(skiplist_itor* itor, size_t count);
/** Move |itor| |count| elements back and return whether it is still
 * positioned. It becomes unpositioned if it runs past the first element. */
bool            skiplist_itor_prevn(skiplist_itor* itor, size_t count);
/** Position |itor| on the first element in key order and return true, or
 * unposition it and return false if the list is empty. */
bool            skiplist_itor_first(skiplist_itor* itor);
/** Position |itor| on the last element in key order and return true, or
 * unposition it and return false if the list is empty. */
bool            skiplist_itor_last(skiplist_itor* itor);
/** Position |itor| on |key| and return true, or unposition it and return
 * false if |key| is not present. */
bool            skiplist_itor_search(skiplist_itor* itor, const void* key);
/** Position |itor| on the greatest key less than or equal to |key| and return
 * true, or unposition it and return false if there is none. */
bool            skiplist_itor_search_le(skiplist_itor* itor, const void* key);
/** Position |itor| on the greatest key less than |key| and return true, or
 * unposition it and return false if there is none. */
bool            skiplist_itor_search_lt(skiplist_itor* itor, const void* key);
/** Position |itor| on the least key greater than or equal to |key| and return
 * true, or unposition it and return false if there is none. */
bool            skiplist_itor_search_ge(skiplist_itor* itor, const void* key);
/** Position |itor| on the least key greater than |key| and return true, or
 * unposition it and return false if there is none. */
bool            skiplist_itor_search_gt(skiplist_itor* itor, const void* key);
/** Return the key |itor| is positioned on, or NULL if it is not positioned.
 * The key belongs to the container and must not be modified in a way that
 * changes how it compares. */
const void*     skiplist_itor_key(const skiplist_itor* itor);
/** Return the address of the datum |itor| is positioned on, through which it
 * can be read or replaced, or NULL if it is not positioned. */
void**          skiplist_itor_datum(skiplist_itor* itor);
/** Compare the positions of two iterators over the same list: return a
 * negative value, zero, or a positive value as |it1| is before, at, or after
 * |it2| in key order. An unpositioned iterator compares before every
 * positioned one. */
int             skiplist_itor_compare(const skiplist_itor* it1, const skiplist_itor* it2);
/** Remove the element |itor| is positioned on and unposition |itor|; return
 * false if it was not positioned. No delete function is called, so fetch the
 * key and datum first if they need freeing. See \ref iterator_invalidation
 * for which iterators this invalidates. */
bool            skiplist_itor_remove(skiplist_itor* itor);

END_DECL

#endif /* !LIBDICT_SKIPLIST_H__ */
