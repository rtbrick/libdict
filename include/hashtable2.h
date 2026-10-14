/*
 * libdict -- open-addressing hash-table interface.
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

#ifndef LIBDICT_HASHTABLE2_H__
#define LIBDICT_HASHTABLE2_H__

#include "dict.h"

/** \file
 *
 * Hash table with open addressing and linear probing.
 *
 * Insert, search and remove take expected O(1) time. The table grows
 * automatically when an insert finds the number of elements at two thirds of
 * the number of slots or more. It is not sorted: there are no near searches,
 * no selection of the nth element, and no iterator comparison, and iteration
 * visits the elements in an unspecified order. A removal moves other elements
 * between slots, so it invalidates every iterator over the table. */

BEGIN_DECL

/** A hash table with open addressing. Opaque: create one with
 * hashtable2_new(). */
typedef struct hashtable2 hashtable2;

/** Create an empty hash table that compares keys with |cmp_func| and hashes
 * them with |hash_func| (neither may be NULL), starting with at least
 * |initial_size| slots: the count is rounded up to the next entry of an
 * internal table of primes (11, 17, 37, 67, ... roughly doubling). Returns
 * NULL if memory cannot be allocated. */
hashtable2*     hashtable2_new(dict_compare_func cmp_func, dict_hash_func hash_func, unsigned initial_size);
/** Like hashtable2_new(), but wrapped in a generic ::dict for use with the
 * dict_* macros. Free it with dict_free(). Returns NULL if memory cannot be
 * allocated. */
dict*           hashtable2_dict_new(dict_compare_func cmp_func, dict_hash_func hash_func, unsigned initial_size);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL, and free the table. Returns the number of elements
 * removed. Iterators over the table may afterwards only be freed. */
size_t          hashtable2_free(hashtable2* table, dict_delete_func delete_func);

/** Insert |key| unless an equal key is present. Returns {&datum, true} for a
 * new key, whose datum starts as NULL; the table stores the |key| pointer
 * itself, it does not copy the key. If an equal key is present, returns
 * {&datum, false} with that key's datum, and |key| is not stored and still
 * belongs to the caller. Returns {NULL, false} if memory cannot be
 * allocated. May grow the table first, which invalidates every iterator over
 * it; inserting a key that is already present never does. */
dict_insert_result
                hashtable2_insert(hashtable2* table, void* key);
/** Return the address of the datum stored for |key|, or NULL if |key| is not
 * present. */
void**          hashtable2_search(hashtable2* table, const void* key);
/** Remove the element with key |key|. Returns {key, datum, true}, handing the
 * stored key and datum back to the caller (no delete function is called), or
 * {NULL, NULL, false} if |key| is not present. See \ref
 * iterator_invalidation for which iterators this invalidates. */
dict_remove_result
                hashtable2_remove(hashtable2* table, const void* key);
/** Remove every element, calling |delete_func|(key, datum) for each unless
 * |delete_func| is NULL. Returns the number of elements removed. The table
 * stays usable. See \ref iterator_invalidation for which iterators this
 * invalidates. */
size_t          hashtable2_clear(hashtable2* table, dict_delete_func delete_func);
/** Call |visit|(key, datum, |user_data|) for each element in table order
 * (unspecified), stopping early if |visit| returns false. Returns the number
 * of calls made. */
size_t          hashtable2_traverse(hashtable2* table, dict_visit_func visit, void* user_data);
/** Return the number of elements. Takes O(1) time. */
size_t          hashtable2_count(const hashtable2* table);
/** Return the current number of slots in the table. */
size_t          hashtable2_size(const hashtable2* table);
/** Return the number of occupied slots, which for open addressing equals the
 * number of elements. */
size_t          hashtable2_slots_used(const hashtable2* table);
/** Check the internal invariants of the table and return true if they hold,
 * or print the first failed check to stderr and return false. Meant for
 * testing and debugging; takes O(n) time. */
bool            hashtable2_verify(const hashtable2* table);
/** Rehash the table into at least |size| slots, rounded up as in
 * hashtable2_new(). Returns true on success, or if the table already has
 * that many slots. Returns false, leaving the table unchanged, if memory
 * cannot be allocated, if fewer slots than elements are requested, or if the
 * slot array would not fit in a size_t. A successful resize invalidates
 * every iterator over the table. */
bool            hashtable2_resize(hashtable2* table, unsigned size);

/** An iterator over a hash table with open addressing. Opaque: create one
 * with hashtable2_itor_new(). */
typedef struct hashtable2_itor hashtable2_itor;

/** Create an iterator over |table| that is not positioned on any element yet.
 * Returns NULL if memory cannot be allocated. */
hashtable2_itor* hashtable2_itor_new(hashtable2* table);
/** Like hashtable2_itor_new(), but wrapped in a generic ::dict_itor for use
 * with the dict_itor_* macros. Free it with dict_itor_free(). */
dict_itor*       hashtable2_dict_itor_new(hashtable2* table);
/** Free an iterator. */
void             hashtable2_itor_free(hashtable2_itor* itor);

/** Return whether |itor| is positioned on an element. False also after a
 * modification of the table that invalidated it. */
bool            hashtable2_itor_valid(const hashtable2_itor* itor);
/** Unposition |itor|, so that it is not positioned on any element. */
void            hashtable2_itor_invalidate(hashtable2_itor* itor);
/** Move |itor| to the next element in table order (unspecified) and return
 * true, or unposition it and return false if there is none. An unpositioned
 * iterator stays unpositioned and returns false. */
bool            hashtable2_itor_next(hashtable2_itor* itor);
/** Move |itor| to the previous element in table order (unspecified) and
 * return true, or unposition it and return false if there is none. An
 * unpositioned iterator stays unpositioned and returns false. */
bool            hashtable2_itor_prev(hashtable2_itor* itor);
/** Move |itor| |count| elements forward and return whether it is still
 * positioned. It becomes unpositioned if it runs past the last element. */
bool            hashtable2_itor_nextn(hashtable2_itor* itor, size_t count);
/** Move |itor| |count| elements back and return whether it is still
 * positioned. It becomes unpositioned if it runs past the first element. */
bool            hashtable2_itor_prevn(hashtable2_itor* itor, size_t count);
/** Position |itor| on the first element in table order (unspecified) and
 * return true, or unposition it and return false if the table is empty. */
bool            hashtable2_itor_first(hashtable2_itor* itor);
/** Position |itor| on the last element in table order (unspecified) and
 * return true, or unposition it and return false if the table is empty. */
bool            hashtable2_itor_last(hashtable2_itor* itor);
/** Position |itor| on |key| and return true, or unposition it and return
 * false if |key| is not present. */
bool            hashtable2_itor_search(hashtable2_itor* itor, const void* key);
/** Return the key |itor| is positioned on, or NULL if it is not positioned.
 * The key belongs to the container and must not be modified in a way that
 * changes how it compares. */
const void*     hashtable2_itor_key(const hashtable2_itor* itor);
/** Return the address of the datum |itor| is positioned on, through which it
 * can be read or replaced, or NULL if it is not positioned. */
void**          hashtable2_itor_datum(hashtable2_itor* itor);
/** Remove the element |itor| is positioned on and unposition |itor|; return
 * false if it was not positioned. No delete function is called, so fetch the
 * key and datum first if they need freeing. See \ref iterator_invalidation
 * for which iterators this invalidates. */
bool            hashtable2_itor_remove(hashtable2_itor* itor);

END_DECL

#endif /* !LIBDICT_HASHTABLE2_H__ */
