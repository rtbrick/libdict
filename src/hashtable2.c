/*
 * libdict -- open-addressing hash-table implementation.
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

/*
 * cf. [Gonnet 1984], [Knuth 1998]
 */

#include "hashtable2.h"

#include <string.h> /* For memset() */
#include "dict_private.h"
#include "hashtable_common.h"

/* TODO: make this configurable in the constructor methods */
#define LOADFACTOR_NUMERATOR    2
#define LOADFACTOR_DENOMINATOR  3
#if LOADFACTOR_NUMERATOR > LOADFACTOR_DENOMINATOR
# error LOADFACTOR_NUMERATOR must be less than LOADFACTOR_DENOMINATOR
#endif

typedef struct hash_node hash_node;

struct hash_node {
    void*               key;
    void*               datum;
    unsigned            hash; /* Untruncated hash value; 0 iff unoccupied. */
};

struct hashtable2 {
    size_t              count;
    dict_compare_func   cmp_func;
    dict_hash_func      hash_func;
    hash_node*          table;
    unsigned            size;
    /* Bumped by every structural modification. Iterators record it so they can
     * detect that the slot they cached no longer holds the element they were
     * positioned on -- removals both empty slots and relocate other elements
     * via the backward shift in remove_cleanup(). */
    unsigned            gen;
};

struct hashtable2_itor {
    hashtable2*         table;
    /* Negative means "not positioned"; otherwise an index into table->table.
     * Signed 64-bit so that every unsigned slot index is representable. */
    ptrdiff_t           slot;
    unsigned            gen;
};

/* Vtable thunks. The vtable slot types take void*, and calling a function
 * through a function-pointer type that is not compatible with the function's
 * own type is undefined behaviour (C11 6.5.2.2p9), so each slot below gets a
 * thunk with the exact slot signature instead of a cast. A cast would also
 * hide a genuine signature mismatch. */
static dict_itor*         vt_hashtable2_dict_itor_new(void* obj)                                    { return hashtable2_dict_itor_new(obj); }
static size_t             vt_hashtable2_free(void* obj, dict_delete_func delete_func)               { return hashtable2_free(obj, delete_func); }
static dict_insert_result vt_hashtable2_insert(void* obj, void* key)                                { return hashtable2_insert(obj, key); }
static void**             vt_hashtable2_search(void* obj, const void* key)                          { return hashtable2_search(obj, key); }
static dict_remove_result vt_hashtable2_remove(void* obj, const void* key)                          { return hashtable2_remove(obj, key); }
static size_t             vt_hashtable2_clear(void* obj, dict_delete_func delete_func)              { return hashtable2_clear(obj, delete_func); }
static size_t             vt_hashtable2_traverse(void* obj, dict_visit_func visit, void* user_data) { return hashtable2_traverse(obj, visit, user_data); }
static size_t             vt_hashtable2_count(const void* obj)                                      { return hashtable2_count(obj); }
static bool               vt_hashtable2_verify(const void* obj)                                     { return hashtable2_verify(obj); }
static void               vt_hashtable2_itor_free(void* itor)                                       { hashtable2_itor_free(itor); }
static bool               vt_hashtable2_itor_valid(const void* itor)                                { return hashtable2_itor_valid(itor); }
static void               vt_hashtable2_itor_invalidate(void* itor)                                 { hashtable2_itor_invalidate(itor); }
static bool               vt_hashtable2_itor_next(void* itor)                                       { return hashtable2_itor_next(itor); }
static bool               vt_hashtable2_itor_prev(void* itor)                                       { return hashtable2_itor_prev(itor); }
static bool               vt_hashtable2_itor_nextn(void* itor, size_t count)                        { return hashtable2_itor_nextn(itor, count); }
static bool               vt_hashtable2_itor_prevn(void* itor, size_t count)                        { return hashtable2_itor_prevn(itor, count); }
static bool               vt_hashtable2_itor_first(void* itor)                                      { return hashtable2_itor_first(itor); }
static bool               vt_hashtable2_itor_last(void* itor)                                       { return hashtable2_itor_last(itor); }
static const void*        vt_hashtable2_itor_key(const void* itor)                                  { return hashtable2_itor_key(itor); }
static void**             vt_hashtable2_itor_datum(void* itor)                                      { return hashtable2_itor_datum(itor); }
static bool               vt_hashtable2_itor_search(void* itor, const void* key)                    { return hashtable2_itor_search(itor, key); }
static bool               vt_hashtable2_itor_remove(void* itor)                                     { return hashtable2_itor_remove(itor); }
static int                vt_hashtable2_itor_compare(const void* itor1, const void* itor2)          { return hashtable2_itor_compare(itor1, itor2); }

static const dict_vtable hashtable2_vtable = {
    false,
    vt_hashtable2_dict_itor_new,
    vt_hashtable2_free,
    vt_hashtable2_insert,
    vt_hashtable2_search,
    NULL,/* search_le: not supported */
    NULL,/* search_lt: not supported */
    NULL,/* search_ge: not supported */
    NULL,/* search_gt: not supported */
    vt_hashtable2_remove,
    vt_hashtable2_clear,
    vt_hashtable2_traverse,
    NULL,
    vt_hashtable2_count,
    vt_hashtable2_verify,
};

static const itor_vtable hashtable2_itor_vtable = {
    vt_hashtable2_itor_free,
    vt_hashtable2_itor_valid,
    vt_hashtable2_itor_invalidate,
    vt_hashtable2_itor_next,
    vt_hashtable2_itor_prev,
    vt_hashtable2_itor_nextn,
    vt_hashtable2_itor_prevn,
    vt_hashtable2_itor_first,
    vt_hashtable2_itor_last,
    vt_hashtable2_itor_key,
    vt_hashtable2_itor_datum,
    vt_hashtable2_itor_search,
    NULL,/* itor_search_le: not supported */
    NULL,/* itor_search_lt: not supported */
    NULL,/* itor_search_ge: not supported */
    NULL,/* itor_search_gt: not supported */
    vt_hashtable2_itor_remove,
    vt_hashtable2_itor_compare,
};

hashtable2*
hashtable2_new(dict_compare_func cmp_func, dict_hash_func hash_func, unsigned initial_size)
{
    ASSERT(cmp_func != NULL);
    ASSERT(hash_func != NULL);
    ASSERT(initial_size > 0);

    hashtable2* table = MALLOC(sizeof(*table));
    if (table) {
        table->size = dict_prime_geq(initial_size);
        /* Guard the byte count against wrapping size_t. Unreachable where
         * size_t is wider than unsigned, but on ILP32 an unchecked product
         * would allocate a small table and then index past it. */
        const size_t bytes = (size_t) table->size * sizeof(hash_node);
        if (bytes / sizeof(hash_node) != (size_t) table->size) {
            FREE(table);
            return NULL;
        }
        table->table = MALLOC(bytes);
        if (!table->table) {
            FREE(table);
            return NULL;
        }
        memset(table->table, 0, bytes);
        table->cmp_func = cmp_func;
        table->hash_func = hash_func;
        table->count = 0;
        table->gen = 0;
    }
    return table;
}

dict*
hashtable2_dict_new(dict_compare_func cmp_func, dict_hash_func hash_func, unsigned initial_size)
{
    ASSERT(cmp_func != NULL);
    ASSERT(hash_func != NULL);
    ASSERT(initial_size > 0);

    dict* dct = MALLOC(sizeof(*dct));
    if (dct) {
        dct->_object = hashtable2_new(cmp_func, hash_func, initial_size);
        if (!dct->_object) {
            FREE(dct);
            return NULL;
        }
        dct->_vtable = &hashtable2_vtable;
    }
    return dct;
}

size_t
hashtable2_free(hashtable2* table, dict_delete_func delete_func)
{
    ASSERT(table != NULL);

    size_t count = hashtable2_clear(table, delete_func);
    FREE(table->table);
    FREE(table);
    return count;
}

static inline dict_insert_result
insert(hashtable2* table, void *key, unsigned hash)
{
    hash_node* const first = table->table + (hash % table->size);
    hash_node* const table_end = table->table + table->size;
    hash_node* node = first;
    do {
        if (!node->hash) {
            node->hash = hash;
            node->key = key;
            ASSERT(node->datum == NULL);
            return (dict_insert_result) { &node->datum, true };
        }

        if (node->hash == hash && table->cmp_func(key, node->key) == 0)
            return (dict_insert_result) { &node->datum, false };

        if (++node == table_end)
            node = table->table;
    } while (node != first);
    /* No room for new element! */
    return (dict_insert_result) { NULL, false };
}

static inline unsigned
nonzero_hash(dict_hash_func hash_func, const void *key)
{
    const unsigned hash = hash_func(key);
    return hash ? hash : ~(unsigned)0;
}

dict_insert_result
hashtable2_insert(hashtable2* table, void* key)
{
    if (LOADFACTOR_DENOMINATOR * table->count >= LOADFACTOR_NUMERATOR * table->size) {
        /* Load factor too high: increase the table size. */
        if (!hashtable2_resize(table, table->size + 1)) {
            /* No memory for a bigger table, but let the insert proceed anyway. */
        }
    }
    const unsigned hash = nonzero_hash(table->hash_func, key);
    dict_insert_result result = insert(table, key, hash);
    if (result.inserted) {
        table->count++;
        table->gen++;
    }
    return result;
}

void**
hashtable2_search(hashtable2* table, const void* key)
{
    const unsigned hash = nonzero_hash(table->hash_func, key);
    hash_node* const first = table->table + (hash % table->size);
    hash_node* const table_end = table->table + table->size;
    hash_node* node = first;
    do {
        if (!node->hash) /* Not occupied. */
            break;

        if (node->hash == hash && table->cmp_func(key, node->key) == 0)
            return &node->datum;

        if (++node == table_end)
            node = table->table;
    } while (node != first);
    return NULL;
}

static void
remove_cleanup(hashtable2* table, hash_node* const first, hash_node* node)
{
    hash_node* const table_end = table->table + table->size;
    do {
        const unsigned hash = node->hash;
        if (!hash) /* Not occupied. */
            break;

        void* const datum = node->datum;
        node->datum = NULL;
        node->hash = 0;

        dict_insert_result result = insert(table, node->key, hash);
        ASSERT(result.inserted);
        ASSERT(result.datum_ptr != NULL);
        ASSERT(*result.datum_ptr == NULL);
        *result.datum_ptr = datum;

        if (++node == table_end)
            node = table->table;
    } while (node != first);
}

static void
remove_node(hashtable2* table, hash_node* first, hash_node* node)
{
    ASSERT(node->hash != 0);

    node->key = node->datum = NULL;
    node->hash = 0;
    table->count--;
    table->gen++;

    if (++node == table->table + table->size)
        node = table->table;
    remove_cleanup(table, first, node);
}

dict_remove_result
hashtable2_remove(hashtable2* table, const void* key)
{
    const unsigned hash = nonzero_hash(table->hash_func, key);
    hash_node* const first = table->table + (hash % table->size);
    hash_node* const table_end = table->table + table->size;
    hash_node* node = first;
    do {
        if (!node->hash) /* Not occupied. */
            break;

        if (node->hash == hash && table->cmp_func(key, node->key) == 0) {
            dict_remove_result result = { node->key, node->datum, true };
            remove_node(table, first, node);
            return result;
        }

        if (++node == table_end)
            node = table->table;
    } while (node != first);
    return (dict_remove_result) { NULL, NULL, false };
}

size_t
hashtable2_clear(hashtable2* table, dict_delete_func delete_func)
{
    hash_node *node = table->table;
    hash_node *const end = table->table + table->size;
    for (; node != end; ++node) {
        if (node->hash) {
            if (delete_func) delete_func(node->key, node->datum);
            node->key = node->datum = NULL;
            node->hash = 0;
        }
    }

    const size_t count = table->count;
    table->count = 0;
    if (count)
        table->gen++;
    return count;
}

size_t
hashtable2_traverse(hashtable2* table, dict_visit_func visit, void* user_data)
{
    size_t count = 0;
    hash_node *node = table->table;
    for (hash_node *const end = table->table + table->size; node != end; ++node) {
        if (node->hash) {
            ++count;
            if (!visit(node->key, node->datum, user_data)) break;
        }
    }
    return count;
}

size_t
hashtable2_count(const hashtable2* table)
{
    return table->count;
}

size_t
hashtable2_size(const hashtable2* table)
{
    return table->size;
}

size_t
hashtable2_slots_used(const hashtable2* table)
{
    return table->count;
}

bool
hashtable2_resize(hashtable2* table, unsigned new_size)
{
    ASSERT(new_size > 0);

    new_size = dict_prime_geq(new_size);
    if (table->size == new_size)
        return true;

    if (table->count > new_size) {
        /* The number of records already in hashtable will not fit (must be a reduction in size). */
        return false;
    }

    /* Guard the byte count against wrapping size_t (see hashtable2_new). */
    const size_t nbytes = (size_t) new_size * sizeof(hash_node);
    if (nbytes / sizeof(hash_node) != (size_t) new_size)
        return false;

    const unsigned old_size = table->size;
    const size_t old_count = table->count;
    hash_node* const old_table = table->table;

    table->table = MALLOC(nbytes);
    if (!table->table) {
        table->table = old_table;
        return false;
    }
    memset(table->table, 0, nbytes);
    table->size = new_size;

    for (unsigned i = 0; i < old_size; ++i) {
        if (old_table[i].hash) {
            dict_insert_result result =
            insert(table, old_table[i].key, old_table[i].hash);
            if (!result.inserted || !result.datum_ptr) {
                FREE(table->table);
                table->table = old_table;
                table->size = old_size;
                table->count = old_count;
                return false;
            }
            *result.datum_ptr = old_table[i].datum;
        }
    }
    FREE(old_table);
    /* Every element was re-homed, so cached slots are meaningless. */
    table->gen++;
    return true;
}

bool
hashtable2_verify(const hashtable2* table)
{
    size_t count = 0;
    const hash_node *node = table->table;
    const hash_node *end = table->table + table->size;
    for (; node != end; ++node) {
        if (node->hash) {
            ++count;
        } else {
            VERIFY(node->datum == NULL);
        }
    }
    VERIFY(table->count == count);
    return true;
}

hashtable2_itor*
hashtable2_itor_new(hashtable2* table)
{
    hashtable2_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        itor->table = table;
        itor->slot = -1;
        itor->gen = table->gen;
    }
    return itor;
}

dict_itor*
hashtable2_dict_itor_new(hashtable2* table)
{
    dict_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        if (!(itor->_itor = hashtable2_itor_new(table))) {
            FREE(itor);
            return NULL;
        }
        itor->_vtable = &hashtable2_itor_vtable;
    }
    return itor;
}

void
hashtable2_itor_free(hashtable2_itor* itor)
{
    FREE(itor);
}

/* True only if the iterator still designates a live element. The slot must be
 * re-checked rather than assumed occupied: a removal can empty it, and
 * remove_cleanup()'s backward shift can move a *different* element into it. */
bool
hashtable2_itor_valid(const hashtable2_itor* itor)
{
    return itor->slot >= 0
        && itor->gen == itor->table->gen
        && itor->table->table[itor->slot].hash != 0;
}

void
hashtable2_itor_invalidate(hashtable2_itor* itor)
{
    itor->slot = -1;
    itor->gen = itor->table->gen;
}

bool
hashtable2_itor_next(hashtable2_itor* itor)
{
    if (!hashtable2_itor_valid(itor))
        return false;

    while (++itor->slot < (ptrdiff_t) itor->table->size) {
        if (itor->table->table[itor->slot].hash)
            return true;
    }
    itor->slot = -1;
    return false;
}

bool
hashtable2_itor_prev(hashtable2_itor* itor)
{
    if (!hashtable2_itor_valid(itor))
        return false;

    while (itor->slot-- > 0) {
        if (itor->table->table[itor->slot].hash)
            return true;
    }
    ASSERT(itor->slot == -1);
    return false;
}

bool
hashtable2_itor_nextn(hashtable2_itor* itor, size_t count)
{
    while (count--)
        if (!hashtable2_itor_next(itor))
            return false;
    return hashtable2_itor_valid(itor);
}

bool
hashtable2_itor_prevn(hashtable2_itor* itor, size_t count)
{
    while (count--)
        if (!hashtable2_itor_prev(itor))
            return false;
    return hashtable2_itor_valid(itor);
}

bool
hashtable2_itor_first(hashtable2_itor* itor)
{
    itor->gen = itor->table->gen;
    for (unsigned slot = 0; slot < itor->table->size; ++slot) {
        if (itor->table->table[slot].hash) {
            itor->slot = (ptrdiff_t) slot;
            return true;
        }
    }
    itor->slot = -1;
    return false;
}

bool
hashtable2_itor_last(hashtable2_itor* itor)
{
    itor->gen = itor->table->gen;
    for (unsigned slot = itor->table->size; slot > 0;) {
        if (itor->table->table[--slot].hash) {
            itor->slot = (ptrdiff_t) slot;
            return true;
        }
    }
    itor->slot = -1;
    return false;
}

bool
hashtable2_itor_search(hashtable2_itor* itor, const void* key)
{
    itor->gen = itor->table->gen;
    const unsigned hash = nonzero_hash(itor->table->hash_func, key);
    const unsigned truncated_hash = hash % itor->table->size;
    unsigned index = truncated_hash;
    do {
        hash_node *node = &itor->table->table[index];
        if (!node->hash)
            break;
        if (node->hash == hash && itor->table->cmp_func(key, node->key) == 0) {
            itor->slot = (ptrdiff_t) index;
            return true;
        }
        if (++index == itor->table->size)
            index = 0;
    } while (index != truncated_hash);
    itor->slot = -1;
    return false;
}

const void*
hashtable2_itor_key(const hashtable2_itor* itor)
{
    return hashtable2_itor_valid(itor) ? itor->table->table[itor->slot].key : NULL;
}

void**
hashtable2_itor_datum(hashtable2_itor* itor)
{
    return hashtable2_itor_valid(itor) ? &itor->table->table[itor->slot].datum : NULL;
}

bool
hashtable2_itor_remove(hashtable2_itor* itor)
{
    if (!hashtable2_itor_valid(itor))
        return false;
    remove_node(itor->table,
        itor->table->table + itor->table->table[itor->slot].hash % itor->table->size,
        itor->table->table + itor->slot);
    itor->slot = -1;
    itor->gen = itor->table->gen;
    return true;
}

/* A hashtable is unordered, so iterators are ordered by slot index, which is
 * the order hashtable2_itor_next() walks. That order is only meaningful until
 * the next structural modification of the table. */
int
hashtable2_itor_compare(const hashtable2_itor* i1, const hashtable2_itor* i2)
{
    ASSERT(i1->table == i2->table);

    const bool v1 = hashtable2_itor_valid(i1), v2 = hashtable2_itor_valid(i2);
    if (!v1)
        return !v2 ? 0 : -1;
    if (!v2)
        return 1;
    return (i1->slot > i2->slot) - (i1->slot < i2->slot);
}

