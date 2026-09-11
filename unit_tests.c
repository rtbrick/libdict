/* unit_tests.c
 * Copyright (C) 2012 Farooq Mela. All rights reserved. */

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "dict.h"
#include "util.h"
#include "src/hashtable_common.h"   /* For dict_prime_geq() */

#define TEST_FUNC(func) { (char *)#func, func }

struct key_info {
    char *key, *value, *alt;
};

struct closest_lookup_info {
    const char *key;
    const char *le_key, *le_val;
    const char *lt_key, *lt_val;
    const char *ge_key, *ge_val;
    const char *gt_key, *gt_val;
};

void test_basic(dict *dct, const struct key_info *keys, const unsigned nkeys, bool keys_sorted);
void test_basic_hashtable_1bucket(void);
void test_basic_hashtable2_1bucket(void);
void test_basic_hashtable_nbuckets(void);
void test_basic_hashtable2_nbuckets(void);
void test_basic_height_balanced_tree(void);
void test_basic_path_reduction_tree(void);
void test_basic_red_black_tree(void);
void test_basic_skiplist(void);
void test_basic_splay_tree(void);
void test_basic_treap(void);
void test_basic_weight_balanced_tree(void);
void test_search(dict *dct, dict_itor *itor, const char *key, const char *value);
void test_closest_lookup(dict *dct, const struct key_info *keys, unsigned nkeys);
void test_traverse(void);
void test_itor_nextn_prevn(void);
void test_itor_invalidate(void);
void test_itor_compare_unsorted(void);
void test_select_null_out(void);
void test_delete_func(void);
void test_comparators(void);
void test_primes_geq(void);
void test_version_string(void);

static CU_TestInfo basic_tests[] = {
    TEST_FUNC(test_basic_hashtable_1bucket),
    TEST_FUNC(test_basic_hashtable2_1bucket),
    TEST_FUNC(test_basic_hashtable_nbuckets),
    TEST_FUNC(test_basic_hashtable2_nbuckets),
    TEST_FUNC(test_basic_height_balanced_tree),
    TEST_FUNC(test_basic_path_reduction_tree),
    TEST_FUNC(test_basic_red_black_tree),
    TEST_FUNC(test_basic_skiplist),
    TEST_FUNC(test_basic_splay_tree),
    TEST_FUNC(test_basic_treap),
    TEST_FUNC(test_basic_weight_balanced_tree),
    TEST_FUNC(test_traverse),
    TEST_FUNC(test_itor_nextn_prevn),
    TEST_FUNC(test_itor_invalidate),
    TEST_FUNC(test_itor_compare_unsorted),
    TEST_FUNC(test_select_null_out),
    TEST_FUNC(test_delete_func),
    TEST_FUNC(test_comparators),
    TEST_FUNC(test_primes_geq),
    TEST_FUNC(test_version_string),
    CU_TEST_INFO_NULL
};

#define TEST_SUITE(suite) { .pName = (char *)#suite, .pTests = suite }

static CU_SuiteInfo test_suites[] = {
    TEST_SUITE(basic_tests),
    CU_SUITE_INFO_NULL
};

static size_t bytes_malloced = 0;

static void*
custom_malloc(size_t n)
{
    size_t* p = malloc(sizeof(size_t) + n);
    if (!p) {
        /* Must not rely on assert(): it disappears under -DNDEBUG, and the
         * store to p[0] below would then write through a NULL pointer. */
        fprintf(stderr, "custom_malloc: out of memory allocating %zu bytes\n", n);
        abort();
    }
    bytes_malloced += n;
    p[0] = n;
    /* NOTE: the returned block is only sizeof(size_t)-aligned, which is weaker
     * than malloc()'s guarantee. That is acceptable here because libdict only
     * ever stores pointer-sized payloads in the memory it allocates. */
    return &p[1];
}

static void
custom_free(void* p)
{
    if (!p) {
        fprintf(stderr, "custom_free: NULL pointer\n");
        abort();
    }
    size_t* sp = p;
    bytes_malloced -= sp[-1];
    free(sp - 1);
}

int
main(void)
{
    dict_malloc_func = custom_malloc;
    dict_free_func = custom_free;

    if (CU_initialize_registry() != CUE_SUCCESS ||
        CU_register_suites(test_suites) != CUE_SUCCESS) {
        fprintf(stderr, "Failed to initialize tests!\n");
        exit(EXIT_FAILURE);
    }
    CU_basic_set_mode(CU_BRM_SILENT);
    if (CU_basic_run_tests() != CUE_SUCCESS) {
        fprintf(stderr, "Failed to run tests!\n");
        exit(EXIT_FAILURE);
    }
    unsigned failures = 0;
    for (const CU_FailureRecord *f = CU_get_failure_list(); f; f = f->pNext) {
        fprintf(stderr, "%u: %s (%s:%u): %s\n",
                ++failures, f->pTest->pName, f->strFileName, f->uiLineNumber, f->strCondition);
    }
    CU_cleanup_registry();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

static const struct key_info unsorted_keys[] = {
    { "d", "D", "d" },
    { "b", "B", "b" },
    { "a", "A", "a" },
    { "c", "C", "c" },
    { "g", "G", "g" },
    { "f", "F", "f" },
    { "h", "H", "h" },
    { "y", "Y", "y" },
    { "z", "Z", "z" },
    { "x", "X", "x" },
    { "j", "J", "j" },
    { "r", "R", "r" },
    { "q", "Q", "q" },
    { "p", "P", "p" },
    { "l", "L", "l" },
    { "m", "M", "m" },
    { "s", "S", "s" },
    { "t", "T", "t" },
    { "u", "U", "u" },
    { "da", "DA", "da" },
    { "ba", "BA", "ba" },
    { "aa", "AA", "aa" },
    { "ca", "CA", "ca" },
    { "ga", "GA", "ga" },
    { "fa", "FA", "fa" },
    { "ha", "HA", "ha" },
    { "ya", "YA", "ya" },
    { "za", "ZA", "za" },
    { "xa", "XA", "xa" },
    { "ja", "JA", "ja" },
    { "ra", "RA", "ra" },
    { "qa", "QA", "qa" },
    { "pa", "PA", "pa" },
    { "la", "LA", "la" },
    { "ma", "MA", "ma" },
    { "sa", "SA", "sa" },
    { "ta", "TA", "ta" },
    { "ua", "UA", "ua" },
};
#define NUM_UNSORTED_KEYS (sizeof(unsorted_keys) / sizeof(unsorted_keys[0]))

static const struct key_info sorted_keys[] = {
    { "a", "A", "a" },
    { "aa", "AA", "aa" },
    { "b", "B", "b" },
    { "ba", "BA", "ba" },
    { "c", "C", "c" },
    { "ca", "CA", "ca" },
    { "d", "D", "d" },
    { "da", "DA", "da" },
    { "f", "F", "f" },
    { "fa", "FA", "fa" },
    { "g", "G", "g" },
    { "ga", "GA", "ga" },
    { "h", "H", "h" },
    { "ha", "HA", "ha" },
    { "j", "J", "j" },
    { "ja", "JA", "ja" },
    { "l", "L", "l" },
    { "la", "LA", "la" },
    { "m", "M", "m" },
    { "ma", "MA", "ma" },
    { "p", "P", "p" },
    { "pa", "PA", "pa" },
    { "q", "Q", "q" },
    { "qa", "QA", "qa" },
    { "r", "R", "r" },
    { "ra", "RA", "ra" },
    { "s", "S", "s" },
    { "sa", "SA", "sa" },
    { "t", "T", "t" },
    { "ta", "TA", "ta" },
    { "u", "U", "u" },
    { "ua", "UA", "ua" },
    { "x", "X", "x" },
    { "xa", "XA", "xa" },
    { "y", "Y", "y" },
    { "ya", "YA", "ya" },
    { "z", "Z", "z" },
    { "za", "ZA", "za" },
};
#define NUM_SORTED_KEYS (sizeof(sorted_keys) / sizeof(sorted_keys[0]))

static_assert(NUM_SORTED_KEYS == NUM_UNSORTED_KEYS, "sorted keys and unsorted keys count mismatch");

static const struct closest_lookup_info closest_lookup_infos[] = {
    {.key = "_",
     .ge_key = "a", .ge_val = "A",
     .gt_key = "a", .gt_val = "A"},
    {.key = "a",
     .le_key = "a", .le_val = "A",
     .ge_key = "a", .ge_val = "A",
     .gt_key = "aa", .gt_val = "AA"},
    {.key = "aa",
     .le_key = "aa", .le_val = "AA",
     .lt_key = "a", .lt_val = "A",
     .ge_key = "aa", .ge_val = "AA",
     .gt_key = "b", .gt_val = "B"},
    {.key = "ab",
     .le_key = "aa", .le_val = "AA",
     .lt_key = "aa", .lt_val = "AA",
     .ge_key = "b", .ge_val = "B",
     .gt_key = "b", .gt_val = "B"},
    {.key = "m",
     .le_key = "m", .le_val = "M",
     .lt_key = "la", .lt_val = "LA",
     .ge_key = "m", .ge_val = "M",
     .gt_key = "ma", .gt_val = "MA"},
    {.key = "n",
     .le_key = "ma", .le_val = "MA",
     .lt_key = "ma", .lt_val = "MA",
     .ge_key = "p", .ge_val = "P",
     .gt_key = "p", .gt_val = "P"},
    {.key = "za",
     .le_key = "za", .le_val = "ZA",
     .lt_key = "z", .lt_val = "Z",
     .ge_key = "za", .ge_val = "ZA"},
    {.key = "zb",
     .le_key = "za", .le_val = "ZA",
     .lt_key = "za", .lt_val = "ZA"},
};

#define NUM_CLOSEST_LOOKUP_INFOS (sizeof(closest_lookup_infos) / sizeof(closest_lookup_infos[0]))

void
test_search(dict *dct, dict_itor *itor, const char *key, const char *value)
{
    void **search = dict_search(dct, key);
    if (value == NULL) {
        CU_ASSERT_PTR_NULL(search);
    } else {
        CU_ASSERT_PTR_NOT_NULL(search);
        CU_ASSERT_EQUAL(*search, value);
    }

    if (itor != NULL) {
        if (value == NULL) {
            CU_ASSERT_FALSE(dict_itor_search(itor, key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
        } else {
            CU_ASSERT_TRUE(dict_itor_search(itor, key));
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_EQUAL(dict_itor_key(itor), key);
            CU_ASSERT_PTR_NOT_NULL(dict_itor_datum(itor));
            CU_ASSERT_EQUAL(*dict_itor_datum(itor), value);
        }
    }
}

/* Identify a container by its vtable. The slots hold file-local thunks with
 * the exact slot signatures rather than the public typed functions (calling a
 * function through an incompatible function-pointer type is undefined
 * behaviour), so a slot can no longer be compared against a public function.
 * Every dict of a given type shares one static vtable, so compare against the
 * vtable of a throwaway instance instead. */
static const dict_vtable*
reference_vtable(dict *dct)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
    const dict_vtable* vtable = dct->_vtable;
    dict_free(dct, NULL);
    return vtable;
}

/* True if `dct` is a chained hashtable. */
static bool
is_hashtable(const dict *dct)
{
    static const dict_vtable* vtable = NULL;
    if (!vtable)
        vtable = reference_vtable(hashtable_dict_new(dict_str_cmp,
                                                     dict_str_hash, 1));
    return dct->_vtable == vtable;
}

/* True if `dct` is an open-addressing hashtable. */
static bool
is_hashtable2(const dict *dct)
{
    static const dict_vtable* vtable = NULL;
    if (!vtable)
        vtable = reference_vtable(hashtable2_dict_new(dict_str_cmp,
                                                      dict_str_hash, 1));
    return dct->_vtable == vtable;
}

/* Compute the expected le/lt/ge/gt answers for `probe` over the first `nkeys`
 * entries of `keys` -- i.e. exactly the contents of the dictionary under test.
 * A NULL result key means "no such element". */
static void
compute_closest(const struct key_info *keys, unsigned nkeys, const char *probe,
        struct closest_lookup_info *out)
{
    memset(out, 0, sizeof(*out));
    out->key = probe;
    for (unsigned i = 0; i < nkeys; ++i) {
        const int cmp = strcmp(keys[i].key, probe);
        if (cmp <= 0 && (!out->le_key || strcmp(keys[i].key, out->le_key) > 0)) {
            out->le_key = keys[i].key;
            out->le_val = keys[i].value;
        }
        if (cmp < 0 && (!out->lt_key || strcmp(keys[i].key, out->lt_key) > 0)) {
            out->lt_key = keys[i].key;
            out->lt_val = keys[i].value;
        }
        if (cmp >= 0 && (!out->ge_key || strcmp(keys[i].key, out->ge_key) < 0)) {
            out->ge_key = keys[i].key;
            out->ge_val = keys[i].value;
        }
        if (cmp > 0 && (!out->gt_key || strcmp(keys[i].key, out->gt_key) < 0)) {
            out->gt_key = keys[i].key;
            out->gt_val = keys[i].value;
        }
    }
}

static void
check_expectation_equal(const char *computed_key, const char *computed_val,
        const char *table_key, const char *table_val)
{
    if (table_key == NULL) {
        CU_ASSERT_PTR_NULL(computed_key);
        CU_ASSERT_PTR_NULL(computed_val);
    } else {
        CU_ASSERT_PTR_NOT_NULL(computed_key);
        if (computed_key) {
            CU_ASSERT_STRING_EQUAL(computed_key, table_key);
            CU_ASSERT_STRING_EQUAL(computed_val, table_val);
        }
    }
}

void
test_closest_lookup(dict *dct, const struct key_info *keys, unsigned nkeys)
{
    const bool sorted = dict_is_sorted(dct);
    dict_itor* itor = dict_itor_new(dct);

    /* These vtable expectations are properties of the container type, not of
     * the individual lookup being performed, so they are checked once. */
    if (sorted) {
        /* Every sorted container must provide the near-search operations, on
         * both the dictionary and its iterators. */
        CU_ASSERT_PTR_NOT_NULL(dct->_vtable->search_le);
        CU_ASSERT_PTR_NOT_NULL(dct->_vtable->search_lt);
        CU_ASSERT_PTR_NOT_NULL(dct->_vtable->search_ge);
        CU_ASSERT_PTR_NOT_NULL(dct->_vtable->search_gt);
        CU_ASSERT_PTR_NOT_NULL(itor->_vtable->search_le);
        CU_ASSERT_PTR_NOT_NULL(itor->_vtable->search_lt);
        CU_ASSERT_PTR_NOT_NULL(itor->_vtable->search_ge);
        CU_ASSERT_PTR_NOT_NULL(itor->_vtable->search_gt);
        /* Every sorted container provides select. */
        CU_ASSERT_PTR_NOT_NULL(dct->_vtable->select);
    } else {
        /* Unsorted containers legitimately lack these operations. */
        CU_ASSERT_PTR_NULL(dct->_vtable->search_le);
        CU_ASSERT_PTR_NULL(dct->_vtable->search_lt);
        CU_ASSERT_PTR_NULL(dct->_vtable->search_ge);
        CU_ASSERT_PTR_NULL(dct->_vtable->search_gt);
        CU_ASSERT_PTR_NULL(itor->_vtable->search_le);
        CU_ASSERT_PTR_NULL(itor->_vtable->search_lt);
        CU_ASSERT_PTR_NULL(itor->_vtable->search_ge);
        CU_ASSERT_PTR_NULL(itor->_vtable->search_gt);
        CU_ASSERT_PTR_NULL(dct->_vtable->select);
    }

    if (sorted && dct->_vtable->select) {
        /* Build the expected ascending order of whatever keys the dictionary
         * currently holds, which is the first nkeys entries of `keys`. */
        const struct key_info *order[NUM_SORTED_KEYS];
        for (unsigned i = 0; i < nkeys; ++i)
            order[i] = &keys[i];
        for (unsigned i = 0; i + 1 < nkeys; ++i) {
            unsigned min = i;
            for (unsigned j = i + 1; j < nkeys; ++j)
                if (strcmp(order[j]->key, order[min]->key) < 0)
                    min = j;
            const struct key_info *tmp = order[i];
            order[i] = order[min];
            order[min] = tmp;
        }
        for (unsigned i = 0; i < nkeys; ++i) {
            const void* key = NULL;
            void* datum = NULL;
            CU_ASSERT_TRUE(dict_select(dct, i, &key, &datum));
            CU_ASSERT_EQUAL(key, order[i]->key);
            CU_ASSERT_EQUAL(datum, order[i]->value);
        }
        const void* key = (const void*)"unwritten";
        void* datum = (void*)"unwritten";
        CU_ASSERT_FALSE(dict_select(dct, nkeys, &key, &datum));
        CU_ASSERT_EQUAL(key, (const void*)NULL);
        CU_ASSERT_PTR_NULL(datum);
    }

    for (unsigned i = 0; i < NUM_CLOSEST_LOOKUP_INFOS; i++) {
        const struct closest_lookup_info* const info = &closest_lookup_infos[i];
        if (!sorted) {
            /* The near-search operations must report "not found" rather than
             * misbehaving on containers that do not support them. */
            CU_ASSERT_PTR_NULL(dict_search_le(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_lt(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_ge(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_gt(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_le(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_search_lt(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_search_ge(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_search_gt(itor, info->key));
            continue;
        }

        /* Expectations for the keys actually present, so that this block runs
         * for every nkeys, not just for the complete key set. */
        struct closest_lookup_info expect;
        compute_closest(keys, nkeys, info->key, &expect);
        if (nkeys == NUM_SORTED_KEYS) {
            /* Cross-check the computation against the hand-written table. */
            check_expectation_equal(expect.le_key, expect.le_val, info->le_key, info->le_val);
            check_expectation_equal(expect.lt_key, expect.lt_val, info->lt_key, info->lt_val);
            check_expectation_equal(expect.ge_key, expect.ge_val, info->ge_key, info->ge_val);
            check_expectation_equal(expect.gt_key, expect.gt_val, info->gt_key, info->gt_val);
        }

        if (expect.le_key) {
            void **datum = dict_search_le(dct, info->key);
            CU_ASSERT_PTR_NOT_NULL(datum);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.le_val) == 0);
            CU_ASSERT_TRUE(dict_itor_search_le(itor, info->key));
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), expect.le_key);
            datum = dict_itor_datum(itor);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.le_val) == 0);
        } else {
            CU_ASSERT_PTR_NULL(dict_search_le(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_le(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (expect.lt_key) {
            void **datum = dict_search_lt(dct, info->key);
            CU_ASSERT_PTR_NOT_NULL(datum);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.lt_val) == 0);
            CU_ASSERT_TRUE(dict_itor_search_lt(itor, info->key));
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), expect.lt_key);
            datum = dict_itor_datum(itor);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.lt_val) == 0);
        } else {
            CU_ASSERT_PTR_NULL(dict_search_lt(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_lt(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (expect.ge_key) {
            void **datum = dict_search_ge(dct, info->key);
            CU_ASSERT_PTR_NOT_NULL(datum);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.ge_val) == 0);
            CU_ASSERT_TRUE(dict_itor_search_ge(itor, info->key));
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), expect.ge_key);
            datum = dict_itor_datum(itor);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.ge_val) == 0);
        } else {
            CU_ASSERT_PTR_NULL(dict_search_ge(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_ge(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (expect.gt_key) {
            void **datum = dict_search_gt(dct, info->key);
            CU_ASSERT_PTR_NOT_NULL(datum);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.gt_val) == 0);
            CU_ASSERT_TRUE(dict_itor_search_gt(itor, info->key));
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), expect.gt_key);
            datum = dict_itor_datum(itor);
            CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, expect.gt_val) == 0);
        } else {
            CU_ASSERT_PTR_NULL(dict_search_gt(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_gt(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
    }
    dict_itor_free(itor);
}

void test_basic(dict *dct, const struct key_info *keys, const unsigned nkeys, bool keys_sorted)
{
    CU_ASSERT_TRUE(dict_verify(dct));

    dict_itor *itor = dict_itor_new(dct);
    CU_ASSERT_PTR_NOT_NULL(itor);
    CU_ASSERT_FALSE(dict_itor_valid(itor));

    CU_ASSERT_FALSE(dict_itor_next(itor));
    CU_ASSERT_FALSE(dict_itor_valid(itor));

    CU_ASSERT_FALSE(dict_itor_prev(itor));
    CU_ASSERT_FALSE(dict_itor_valid(itor));

    for (unsigned i = 0; i < nkeys; ++i) {
    dict_insert_result result = dict_insert(dct, keys[i].key);
    CU_ASSERT_TRUE(result.inserted);
    CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
    CU_ASSERT_PTR_NULL(*result.datum_ptr);
    *result.datum_ptr = keys[i].value;

    CU_ASSERT_TRUE(dict_verify(dct));

    for (unsigned j = 0; j <= i; ++j)
        test_search(dct, itor, keys[j].key, keys[j].value);
    for (unsigned j = i + 1; j < nkeys; ++j)
        test_search(dct, itor, keys[j].key, NULL);
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);

    if (is_hashtable(dct) || is_hashtable2(dct)) {
        /* Verify that hashtable_resize works as expected. */
        if (is_hashtable(dct)) {
            CU_ASSERT_TRUE(hashtable_resize(dict_private(dct), 3));
        } else {
            CU_ASSERT_TRUE(hashtable2_resize(dict_private(dct),
                           dict_prime_geq(nkeys * 5)));
        }
        CU_ASSERT_TRUE(dict_verify(dct));
        CU_ASSERT_EQUAL(dict_count(dct), nkeys);
        for (unsigned j = 0; j < nkeys; ++j)
            test_search(dct, NULL, keys[j].key, keys[j].value);
    }

    for (unsigned i = 0; i < nkeys; ++i)
        test_search(dct, itor, keys[i].key, keys[i].value);

    for (unsigned i = 0; i < nkeys; ++i) {
        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_FALSE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        CU_ASSERT_EQUAL(*result.datum_ptr, keys[i].value);

        CU_ASSERT_TRUE(dict_verify(dct));
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);

    dict_itor *first = dict_itor_new(dct);
    CU_ASSERT_PTR_NOT_NULL(first);
    CU_ASSERT_FALSE(dict_itor_valid(first));
    if (nkeys > 0) {
        CU_ASSERT_TRUE(dict_itor_first(first));
        CU_ASSERT_TRUE(dict_itor_valid(first));
    } else {
        CU_ASSERT_FALSE(dict_itor_first(first));
        CU_ASSERT_FALSE(dict_itor_valid(first));
    }

    dict_itor *last = dict_itor_new(dct);
    CU_ASSERT_PTR_NOT_NULL(last);
    CU_ASSERT_FALSE(dict_itor_valid(last));
    if (nkeys > 0) {
        CU_ASSERT_TRUE(dict_itor_last(last));
        CU_ASSERT_TRUE(dict_itor_valid(last));
    } else {
        CU_ASSERT_FALSE(dict_itor_last(last));
        CU_ASSERT_FALSE(dict_itor_valid(last));
    }
    if (dict_is_sorted(dct)) {
        if (nkeys <= 1) {
            CU_ASSERT_TRUE(dict_itor_compare(first, last) == 0);
            CU_ASSERT_TRUE(dict_itor_compare(last, first) == 0);
        } else {
            CU_ASSERT_TRUE(dict_itor_compare(first, last) < 0);
            CU_ASSERT_TRUE(dict_itor_compare(last, first) > 0);
        }
    }

    const char *last_key = NULL;
    unsigned n = 0;
    for (dict_itor_first(itor); dict_itor_valid(itor); dict_itor_next(itor)) {
        CU_ASSERT_PTR_NOT_NULL(dict_itor_key(itor));
        CU_ASSERT_PTR_NOT_NULL(dict_itor_datum(itor));
        CU_ASSERT_PTR_NOT_NULL(*dict_itor_datum(itor));
        if (dict_is_sorted(dct)) {
            CU_ASSERT_TRUE(dict_itor_compare(itor, itor) == 0);
            if (n == 0) {
                CU_ASSERT_TRUE(dict_itor_compare(itor, first) == 0);
                CU_ASSERT_TRUE(dict_itor_compare(first, itor) == 0);
            } else {
                CU_ASSERT_TRUE(dict_itor_compare(itor, first) > 0);
                CU_ASSERT_TRUE(dict_itor_compare(first, itor) < 0);
            }
            if (n == nkeys - 1) {
                CU_ASSERT_TRUE(dict_itor_compare(itor, last) == 0);
                CU_ASSERT_TRUE(dict_itor_compare(last, itor) == 0);
            } else {
                CU_ASSERT_TRUE(dict_itor_compare(itor, last) < 0);
                CU_ASSERT_TRUE(dict_itor_compare(last, itor) > 0);
            }
            if (keys_sorted) {
                CU_ASSERT_EQUAL(dict_itor_key(itor), keys[n].key);
                CU_ASSERT_EQUAL(*dict_itor_datum(itor), keys[n].value);
            }
        }
        unsigned keys_matched = 0;
        for (unsigned i = 0; i < nkeys; ++i) {
            if (dict_itor_key(itor) == keys[i].key) {
                CU_ASSERT_EQUAL(*dict_itor_datum(itor), keys[i].value);
                keys_matched++;
            } else {
                CU_ASSERT_NOT_EQUAL(*dict_itor_datum(itor), keys[i].value);
            }
        }
        CU_ASSERT_EQUAL(keys_matched, 1);

        if (dict_is_sorted(dct)) {
            if (last_key) {
                CU_ASSERT_TRUE(strcmp(last_key, dict_itor_key(itor)) < 0);
            }
            last_key = dict_itor_key(itor);
        }

        ++n;
    }
    CU_ASSERT_EQUAL(n, nkeys);
    last_key = NULL;
    n = 0;
    for (dict_itor_last(itor); dict_itor_valid(itor); dict_itor_prev(itor)) {
        CU_ASSERT_PTR_NOT_NULL(dict_itor_key(itor));
        CU_ASSERT_PTR_NOT_NULL(dict_itor_datum(itor));
        CU_ASSERT_PTR_NOT_NULL(*dict_itor_datum(itor));

        if (dict_is_sorted(dct) && keys_sorted) {
            CU_ASSERT_EQUAL(dict_itor_key(itor), keys[nkeys - 1 - n].key);
            CU_ASSERT_EQUAL(*dict_itor_datum(itor), keys[nkeys - 1 - n].value);
        }
        unsigned keys_matched = 0;
        for (unsigned i = 0; i < nkeys; ++i) {
            if (dict_itor_key(itor) == keys[i].key) {
                CU_ASSERT_EQUAL(*dict_itor_datum(itor), keys[i].value);
                keys_matched++;
            } else {
                CU_ASSERT_NOT_EQUAL(*dict_itor_datum(itor), keys[i].value);
            }
        }
        CU_ASSERT_EQUAL(keys_matched, 1);

        if (dict_is_sorted(dct)) {
            if (last_key) {
                CU_ASSERT_TRUE(strcmp(last_key, dict_itor_key(itor)) > 0);
            }
            last_key = dict_itor_key(itor);
        }

        ++n;
    }
    CU_ASSERT_EQUAL(n, nkeys);

    for (unsigned i = 0; i < nkeys; ++i) {
        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_FALSE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        CU_ASSERT_PTR_NOT_NULL(*result.datum_ptr);
        *result.datum_ptr = keys[i].alt;

        CU_ASSERT_TRUE(dict_verify(dct));
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);

    for (unsigned i = 0; i < nkeys; ++i)
        test_search(dct, itor, keys[i].key, keys[i].alt);

    for (unsigned i = 0; i < nkeys; ++i) {
        test_search(dct, itor, keys[i].key, keys[i].alt);

        dict_remove_result result = dict_remove(dct, keys[i].key);
        CU_ASSERT_TRUE(result.removed);
        CU_ASSERT_EQUAL(result.key, keys[i].key);
        CU_ASSERT_EQUAL(result.datum, keys[i].alt);
        CU_ASSERT_TRUE(dict_verify(dct));

        result = dict_remove(dct, keys[i].key);
        CU_ASSERT_FALSE(result.removed);
        CU_ASSERT_PTR_NULL(result.key);
        CU_ASSERT_PTR_NULL(result.datum);
        for (unsigned j = 0; j <= i; ++j) {
            test_search(dct, itor, keys[j].key, NULL);
        }
        for (unsigned j = i + 1; j < nkeys; ++j) {
            test_search(dct, itor, keys[j].key, keys[j].alt);
        }
    }

    for (unsigned i = 0; i < nkeys; ++i) {
        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_TRUE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        CU_ASSERT_PTR_NULL(*result.datum_ptr);
        *result.datum_ptr = keys[i].value;

        CU_ASSERT_TRUE(dict_verify(dct));
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);
    CU_ASSERT_EQUAL(dict_clear(dct, NULL), nkeys);
    CU_ASSERT_TRUE(dict_verify(dct));
    CU_ASSERT_EQUAL(dict_count(dct), 0);

    for (unsigned i = 0; i < nkeys; ++i) {
        test_search(dct, itor, keys[i].key, NULL);

        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_TRUE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        CU_ASSERT_PTR_NULL(*result.datum_ptr);
        *result.datum_ptr = keys[i].value;

        CU_ASSERT_TRUE(dict_verify(dct));
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);

    test_closest_lookup(dct, keys, nkeys);

    for (unsigned i = 0; i < nkeys; ++i) {
        CU_ASSERT_TRUE(dict_itor_search(itor, keys[i].key));
        CU_ASSERT_TRUE(dict_itor_valid(itor));
        CU_ASSERT_EQUAL(dict_itor_key(itor), keys[i].key);
        CU_ASSERT_PTR_NOT_NULL(dict_itor_datum(itor));
        CU_ASSERT_EQUAL(*dict_itor_datum(itor), keys[i].value);

        CU_ASSERT_TRUE(dict_itor_remove(itor));
        CU_ASSERT_FALSE(dict_itor_valid(itor));

        CU_ASSERT_FALSE(dict_itor_search(itor, keys[i].key));
        CU_ASSERT_FALSE(dict_itor_valid(itor));
    }
    CU_ASSERT_EQUAL(dict_count(dct), 0);

    for (unsigned i = 0; i < nkeys; ++i) {
        test_search(dct, itor, keys[i].key, NULL);

        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_TRUE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        CU_ASSERT_PTR_NULL(*result.datum_ptr);
        *result.datum_ptr = keys[i].value;

        CU_ASSERT_TRUE(dict_verify(dct));
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);

    dict_itor_free(itor);
    dict_itor_free(first);
    dict_itor_free(last);
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);
    CU_ASSERT_EQUAL(dict_free(dct, NULL), nkeys);
}

void test_basic_hashtable_1bucket()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable_dict_new(dict_str_cmp, dict_str_hash, 1), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable_dict_new(dict_str_cmp, dict_str_hash, 1), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_hashtable2_1bucket()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable2_dict_new(dict_str_cmp, dict_str_hash, 1), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable2_dict_new(dict_str_cmp, dict_str_hash, 1), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_hashtable_nbuckets()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable_dict_new(dict_str_cmp, dict_str_hash, 7), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable_dict_new(dict_str_cmp, dict_str_hash, 7), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_hashtable2_nbuckets()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable2_dict_new(dict_str_cmp, dict_str_hash, 7), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hashtable2_dict_new(dict_str_cmp, dict_str_hash, 7), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_height_balanced_tree()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hb_dict_new(dict_str_cmp), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(hb_dict_new(dict_str_cmp), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_path_reduction_tree()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(pr_dict_new(dict_str_cmp), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(pr_dict_new(dict_str_cmp), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_red_black_tree()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(rb_dict_new(dict_str_cmp), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(rb_dict_new(dict_str_cmp), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_skiplist()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(skiplist_dict_new(dict_str_cmp, 13), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(skiplist_dict_new(dict_str_cmp, 13), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_splay_tree()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(sp_dict_new(dict_str_cmp), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(sp_dict_new(dict_str_cmp), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_treap()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(tr_dict_new(dict_str_cmp, NULL), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(tr_dict_new(dict_str_cmp, NULL), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_basic_weight_balanced_tree()
{
    for (unsigned n = 0; n <= NUM_SORTED_KEYS; ++n) {
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(wb_dict_new(dict_str_cmp), unsorted_keys, n, false);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
        test_basic(wb_dict_new(dict_str_cmp), sorted_keys, n, true);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

/* Every container type covered by the basic tests, so that the tests below can
 * be run against all of them. */
static dict *new_hashtable_1bucket(void)
{ return hashtable_dict_new(dict_str_cmp, dict_str_hash, 1); }
static dict *new_hashtable_nbuckets(void)
{ return hashtable_dict_new(dict_str_cmp, dict_str_hash, 7); }
static dict *new_hashtable2_1bucket(void)
{ return hashtable2_dict_new(dict_str_cmp, dict_str_hash, 1); }
static dict *new_hashtable2_nbuckets(void)
{ return hashtable2_dict_new(dict_str_cmp, dict_str_hash, 7); }
static dict *new_hb_tree(void) { return hb_dict_new(dict_str_cmp); }
static dict *new_pr_tree(void) { return pr_dict_new(dict_str_cmp); }
static dict *new_rb_tree(void) { return rb_dict_new(dict_str_cmp); }
static dict *new_skiplist(void) { return skiplist_dict_new(dict_str_cmp, 13); }
static dict *new_sp_tree(void) { return sp_dict_new(dict_str_cmp); }
static dict *new_tr_tree(void) { return tr_dict_new(dict_str_cmp, NULL); }
static dict *new_wb_tree(void) { return wb_dict_new(dict_str_cmp); }

struct container_info {
    const char *name;
    dict *(*new_dict)(void);
};

static const struct container_info containers[] = {
    { "hashtable_1bucket", new_hashtable_1bucket },
    { "hashtable_nbuckets", new_hashtable_nbuckets },
    { "hashtable2_1bucket", new_hashtable2_1bucket },
    { "hashtable2_nbuckets", new_hashtable2_nbuckets },
    { "hb_tree", new_hb_tree },
    { "pr_tree", new_pr_tree },
    { "rb_tree", new_rb_tree },
    { "skiplist", new_skiplist },
    { "sp_tree", new_sp_tree },
    { "tr_tree", new_tr_tree },
    { "wb_tree", new_wb_tree },
};
#define NUM_CONTAINERS (sizeof(containers) / sizeof(containers[0]))

/* Insert the first nkeys entries of `keys` into `dct`. */
static void
fill_dict(dict *dct, const struct key_info *keys, unsigned nkeys)
{
    for (unsigned i = 0; i < nkeys; ++i) {
        dict_insert_result result = dict_insert(dct, keys[i].key);
        CU_ASSERT_TRUE(result.inserted);
        CU_ASSERT_PTR_NOT_NULL(result.datum_ptr);
        if (result.datum_ptr)
            *result.datum_ptr = keys[i].value;
    }
    CU_ASSERT_EQUAL(dict_count(dct), nkeys);
}

struct traverse_state {
    dict *dct;
    bool sorted;
    unsigned visits;
    unsigned stop_after;    /* 0 means never stop early */
    const char *last_key;
};

static bool
traverse_visit(const void *key, void *datum, void *user_data)
{
    struct traverse_state *state = user_data;

    ++state->visits;
    CU_ASSERT_PTR_NOT_NULL(key);
    /* Every visited pair must agree with what a search for the key yields. */
    void **search = dict_search(state->dct, key);
    CU_ASSERT_PTR_NOT_NULL(search);
    if (search)
        CU_ASSERT_PTR_EQUAL(*search, datum);
    if (state->sorted) {
        if (state->last_key)
            CU_ASSERT_TRUE(strcmp(state->last_key, key) < 0);
        state->last_key = key;
    }
    if (state->stop_after && state->visits >= state->stop_after)
        return false;
    return true;
}

void test_traverse()
{
    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        for (unsigned nkeys = 0; nkeys <= NUM_SORTED_KEYS; ++nkeys) {
            for (unsigned pass = 0; pass < 2; ++pass) {
                const struct key_info *keys = pass ? sorted_keys : unsorted_keys;
                dict *dct = containers[c].new_dict();
                CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
                fill_dict(dct, keys, nkeys);

                struct traverse_state state = {
                    .dct = dct, .sorted = dict_is_sorted(dct)
                };
                CU_ASSERT_EQUAL(dict_traverse(dct, traverse_visit, &state), nkeys);
                CU_ASSERT_EQUAL(state.visits, nkeys);

                /* A visit function that returns false after k elements must
                 * stop the traversal and make dict_traverse() return k. */
                for (unsigned k = 1; k <= nkeys; ++k) {
                    struct traverse_state stop_state = {
                        .dct = dct, .sorted = dict_is_sorted(dct), .stop_after = k
                    };
                    CU_ASSERT_EQUAL(dict_traverse(dct, traverse_visit, &stop_state), k);
                    CU_ASSERT_EQUAL(stop_state.visits, k);
                }

                CU_ASSERT_EQUAL(dict_free(dct, NULL), nkeys);
                CU_ASSERT_EQUAL(bytes_malloced, 0);
            }
        }
    }
}

void test_itor_nextn_prevn()
{
    static const unsigned key_counts[] = { 0, 1, 2, 3, 7, NUM_SORTED_KEYS };

    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        for (unsigned ki = 0; ki < sizeof(key_counts) / sizeof(key_counts[0]); ++ki) {
            const unsigned nkeys = key_counts[ki];
            dict *dct = containers[c].new_dict();
            CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
            fill_dict(dct, unsorted_keys, nkeys);

            for (unsigned n = 0; n <= nkeys + 1; ++n) {
                dict_itor *a = dict_itor_new(dct);
                dict_itor *b = dict_itor_new(dct);
                CU_ASSERT_PTR_NOT_NULL_FATAL(a);
                CU_ASSERT_PTR_NOT_NULL_FATAL(b);
                CU_ASSERT_PTR_NOT_NULL(a->_vtable->compare);

                /* nextn(n) from the first element must land where n successive
                 * next() calls land, and agree on the returned bool. */
                CU_ASSERT_EQUAL(dict_itor_first(a), nkeys > 0);
                CU_ASSERT_EQUAL(dict_itor_first(b), nkeys > 0);
                const bool a_ret = dict_itor_nextn(a, n);
                bool b_ret = dict_itor_valid(b);
                for (unsigned j = 0; j < n; ++j) {
                    b_ret = dict_itor_next(b);
                    if (!b_ret)
                        break;
                }
                CU_ASSERT_EQUAL(a_ret, b_ret);
                CU_ASSERT_EQUAL(dict_itor_valid(a), dict_itor_valid(b));
                CU_ASSERT_EQUAL(dict_itor_compare(a, b), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(b, a), 0);
                if (dict_itor_valid(a)) {
                    CU_ASSERT_PTR_EQUAL(dict_itor_key(a), dict_itor_key(b));
                    CU_ASSERT_EQUAL(a_ret, true);
                }

                /* Same for prevn(n) from the last element. */
                CU_ASSERT_EQUAL(dict_itor_last(a), nkeys > 0);
                CU_ASSERT_EQUAL(dict_itor_last(b), nkeys > 0);
                const bool a_prev_ret = dict_itor_prevn(a, n);
                bool b_prev_ret = dict_itor_valid(b);
                for (unsigned j = 0; j < n; ++j) {
                    b_prev_ret = dict_itor_prev(b);
                    if (!b_prev_ret)
                        break;
                }
                CU_ASSERT_EQUAL(a_prev_ret, b_prev_ret);
                CU_ASSERT_EQUAL(dict_itor_valid(a), dict_itor_valid(b));
                CU_ASSERT_EQUAL(dict_itor_compare(a, b), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(b, a), 0);
                if (dict_itor_valid(a)) {
                    CU_ASSERT_PTR_EQUAL(dict_itor_key(a), dict_itor_key(b));
                    CU_ASSERT_EQUAL(a_prev_ret, true);
                }

                dict_itor_free(a);
                dict_itor_free(b);
            }
            CU_ASSERT_EQUAL(dict_free(dct, NULL), nkeys);
            CU_ASSERT_EQUAL(bytes_malloced, 0);
        }
    }
}

void test_itor_invalidate()
{
    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        dict *dct = containers[c].new_dict();
        CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
        fill_dict(dct, unsorted_keys, NUM_SORTED_KEYS);

        dict_itor *itor = dict_itor_new(dct);
        CU_ASSERT_PTR_NOT_NULL_FATAL(itor);
        CU_ASSERT_TRUE(dict_itor_first(itor));
        CU_ASSERT_TRUE(dict_itor_valid(itor));
        dict_itor_invalidate(itor);
        CU_ASSERT_FALSE(dict_itor_valid(itor));
        CU_ASSERT_PTR_NULL(dict_itor_key(itor));
        CU_ASSERT_PTR_NULL(dict_itor_datum(itor));

        /* Invalidating an already invalid iterator is harmless. */
        dict_itor_invalidate(itor);
        CU_ASSERT_FALSE(dict_itor_valid(itor));
        CU_ASSERT_PTR_NULL(dict_itor_key(itor));
        CU_ASSERT_PTR_NULL(dict_itor_datum(itor));

        /* The iterator is still usable afterwards. */
        CU_ASSERT_TRUE(dict_itor_last(itor));
        CU_ASSERT_TRUE(dict_itor_valid(itor));
        CU_ASSERT_PTR_NOT_NULL(dict_itor_key(itor));

        dict_itor_free(itor);
        CU_ASSERT_EQUAL(dict_free(dct, NULL), NUM_SORTED_KEYS);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_itor_compare_unsorted()
{
    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        for (unsigned nkeys = 0; nkeys <= 5; ++nkeys) {
            dict *dct = containers[c].new_dict();
            CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
            /* Ordering assertions specific to sorted containers live in
             * test_basic(); this test covers what holds for every container,
             * sorted or not. */
            fill_dict(dct, unsorted_keys, nkeys);

            dict_itor *first = dict_itor_new(dct);
            dict_itor *last = dict_itor_new(dct);
            dict_itor *invalid = dict_itor_new(dct);
            CU_ASSERT_PTR_NOT_NULL_FATAL(first);
            CU_ASSERT_PTR_NOT_NULL_FATAL(last);
            CU_ASSERT_PTR_NOT_NULL_FATAL(invalid);
            /* Every container -- sorted or not -- must implement compare. */
            CU_ASSERT_PTR_NOT_NULL(first->_vtable->compare);

            CU_ASSERT_EQUAL(dict_itor_first(first), nkeys > 0);
            CU_ASSERT_EQUAL(dict_itor_last(last), nkeys > 0);
            CU_ASSERT_FALSE(dict_itor_valid(invalid));

            /* An iterator always equals itself. */
            CU_ASSERT_EQUAL(dict_itor_compare(first, first), 0);
            CU_ASSERT_EQUAL(dict_itor_compare(last, last), 0);
            CU_ASSERT_EQUAL(dict_itor_compare(invalid, invalid), 0);

            if (nkeys == 0) {
                CU_ASSERT_EQUAL(dict_itor_compare(first, last), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(last, first), 0);
            } else if (nkeys == 1) {
                CU_ASSERT_EQUAL(dict_itor_compare(first, last), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(last, first), 0);
            } else {
                /* Distinct elements compare nonzero with opposite signs. */
                const int cmp = dict_itor_compare(first, last);
                const int rcmp = dict_itor_compare(last, first);
                CU_ASSERT_NOT_EQUAL(cmp, 0);
                CU_ASSERT_TRUE((cmp < 0) == (rcmp > 0));
                /* Repeating the comparison gives the same answer. */
                CU_ASSERT_EQUAL(dict_itor_compare(first, last), cmp);
            }

            if (nkeys > 0) {
                /* An invalid iterator compares consistently against a valid
                 * one, in both directions. */
                const int cmp = dict_itor_compare(invalid, first);
                const int rcmp = dict_itor_compare(first, invalid);
                CU_ASSERT_NOT_EQUAL(cmp, 0);
                CU_ASSERT_NOT_EQUAL(rcmp, 0);
                CU_ASSERT_TRUE((cmp < 0) == (rcmp > 0));

                /* Invalidating an iterator makes it compare like a freshly
                 * created (invalid) one. */
                dict_itor_invalidate(last);
                CU_ASSERT_EQUAL(dict_itor_compare(last, invalid), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(invalid, last), 0);
                CU_ASSERT_EQUAL(dict_itor_compare(last, first), cmp);
                CU_ASSERT_EQUAL(dict_itor_compare(first, last), rcmp);
            }

            dict_itor_free(first);
            dict_itor_free(last);
            dict_itor_free(invalid);
            CU_ASSERT_EQUAL(dict_free(dct, NULL), nkeys);
            CU_ASSERT_EQUAL(bytes_malloced, 0);
        }
    }
}

void test_select_null_out()
{
    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        for (unsigned nkeys = 0; nkeys <= 5; ++nkeys) {
            dict *dct = containers[c].new_dict();
            CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
            if (!dict_is_sorted(dct)) {
                dict_free(dct, NULL);
                continue;
            }
            fill_dict(dct, unsorted_keys, nkeys);
            const bool has_select = dct->_vtable->select != NULL;

            for (unsigned n = 0; n <= nkeys + 1; ++n) {
                const bool expected = has_select && n < nkeys;

                /* Both out-parameters. */
                const void *key = NULL;
                void *datum = NULL;
                CU_ASSERT_EQUAL(dict_select(dct, n, &key, &datum), expected);
                if (expected) {
                    CU_ASSERT_PTR_NOT_NULL(key);
                    CU_ASSERT_PTR_NOT_NULL(datum);
                } else {
                    CU_ASSERT_PTR_NULL(key);
                    CU_ASSERT_PTR_NULL(datum);
                }

                /* Only the datum requested. */
                void *datum_only = NULL;
                CU_ASSERT_EQUAL(dict_select(dct, n, NULL, &datum_only), expected);
                if (expected) {
                    CU_ASSERT_PTR_EQUAL(datum_only, datum);
                } else {
                    CU_ASSERT_PTR_NULL(datum_only);
                }

                /* Only the key requested. */
                const void *key_only = NULL;
                CU_ASSERT_EQUAL(dict_select(dct, n, &key_only, NULL), expected);
                if (expected) {
                    CU_ASSERT_PTR_EQUAL(key_only, key);
                } else {
                    CU_ASSERT_PTR_NULL(key_only);
                }

                /* Neither out-parameter. */
                CU_ASSERT_EQUAL(dict_select(dct, n, NULL, NULL), expected);
            }
            CU_ASSERT_EQUAL(dict_free(dct, NULL), nkeys);
            CU_ASSERT_EQUAL(bytes_malloced, 0);
        }
    }
}

#define NUM_DELETE_KEYS 16

static struct {
    void *keys[NUM_DELETE_KEYS];
    void *data[NUM_DELETE_KEYS];
    unsigned seen[NUM_DELETE_KEYS];
    unsigned calls;
} delete_tracker;

static void
tracking_delete_func(void *key, void *datum)
{
    ++delete_tracker.calls;

    bool found = false;
    for (unsigned i = 0; i < NUM_DELETE_KEYS; ++i) {
        if (delete_tracker.keys[i] == key) {
            CU_ASSERT_PTR_EQUAL(delete_tracker.data[i], datum);
            ++delete_tracker.seen[i];
            found = true;
            break;
        }
    }
    /* The delete function must only ever be handed pointers we inserted. */
    CU_ASSERT_TRUE(found);

    free(key);
    free(datum);
}

static char *
test_strdup(const char *str)
{
    const size_t len = strlen(str) + 1;
    char *copy = malloc(len);
    if (!copy) {
        fprintf(stderr, "test_strdup: out of memory\n");
        abort();
    }
    memcpy(copy, str, len);
    return copy;
}

void test_delete_func()
{
    for (unsigned c = 0; c < NUM_CONTAINERS; ++c) {
        /* pass 0 exercises dict_clear(), pass 1 exercises dict_free(). */
        for (unsigned pass = 0; pass < 2; ++pass) {
            dict *dct = containers[c].new_dict();
            CU_ASSERT_PTR_NOT_NULL_FATAL(dct);

            memset(&delete_tracker, 0, sizeof(delete_tracker));
            for (unsigned i = 0; i < NUM_DELETE_KEYS; ++i) {
                char buf[32];
                snprintf(buf, sizeof(buf), "key%u", i);
                char *key = test_strdup(buf);
                snprintf(buf, sizeof(buf), "value%u", i);
                char *datum = test_strdup(buf);
                delete_tracker.keys[i] = key;
                delete_tracker.data[i] = datum;

                dict_insert_result result = dict_insert(dct, key);
                CU_ASSERT_TRUE(result.inserted);
                CU_ASSERT_PTR_NOT_NULL_FATAL(result.datum_ptr);
                *result.datum_ptr = datum;
            }
            CU_ASSERT_EQUAL(dict_count(dct), NUM_DELETE_KEYS);
            CU_ASSERT_TRUE(dict_verify(dct));

            if (pass == 0) {
                CU_ASSERT_EQUAL(dict_clear(dct, tracking_delete_func), NUM_DELETE_KEYS);
                CU_ASSERT_EQUAL(dict_count(dct), 0);
                CU_ASSERT_TRUE(dict_verify(dct));
                CU_ASSERT_EQUAL(delete_tracker.calls, NUM_DELETE_KEYS);
                /* An empty dictionary must not call the delete function. */
                CU_ASSERT_EQUAL(dict_free(dct, tracking_delete_func), 0);
                CU_ASSERT_EQUAL(delete_tracker.calls, NUM_DELETE_KEYS);
            } else {
                CU_ASSERT_EQUAL(dict_free(dct, tracking_delete_func), NUM_DELETE_KEYS);
                CU_ASSERT_EQUAL(delete_tracker.calls, NUM_DELETE_KEYS);
            }
            /* Exactly one call per element, with the pointers we inserted. */
            for (unsigned i = 0; i < NUM_DELETE_KEYS; ++i)
                CU_ASSERT_EQUAL(delete_tracker.seen[i], 1);
            CU_ASSERT_EQUAL(bytes_malloced, 0);
        }
    }
}

static int
cmp_sign(int value)
{
    return (value > 0) - (value < 0);
}

void test_comparators()
{
    const int ints[] = { INT_MIN, -3, -1, 0, 1, 3, INT_MAX };
    for (unsigned i = 0; i < sizeof(ints) / sizeof(ints[0]); ++i)
        for (unsigned j = 0; j < sizeof(ints) / sizeof(ints[0]); ++j)
            CU_ASSERT_EQUAL(cmp_sign(dict_int_cmp(&ints[i], &ints[j])),
                            (int)(i > j) - (int)(i < j));

    const unsigned uints[] = { 0, 1, 3, INT_MAX, UINT_MAX };
    for (unsigned i = 0; i < sizeof(uints) / sizeof(uints[0]); ++i)
        for (unsigned j = 0; j < sizeof(uints) / sizeof(uints[0]); ++j)
            CU_ASSERT_EQUAL(cmp_sign(dict_uint_cmp(&uints[i], &uints[j])),
                            (int)(i > j) - (int)(i < j));

    const long longs[] = { LONG_MIN, -3, -1, 0, 1, 3, LONG_MAX };
    for (unsigned i = 0; i < sizeof(longs) / sizeof(longs[0]); ++i)
        for (unsigned j = 0; j < sizeof(longs) / sizeof(longs[0]); ++j)
            CU_ASSERT_EQUAL(cmp_sign(dict_long_cmp(&longs[i], &longs[j])),
                            (int)(i > j) - (int)(i < j));

    const unsigned long ulongs[] = { 0, 1, 3, LONG_MAX, ULONG_MAX };
    for (unsigned i = 0; i < sizeof(ulongs) / sizeof(ulongs[0]); ++i)
        for (unsigned j = 0; j < sizeof(ulongs) / sizeof(ulongs[0]); ++j)
            CU_ASSERT_EQUAL(cmp_sign(dict_ulong_cmp(&ulongs[i], &ulongs[j])),
                            (int)(i > j) - (int)(i < j));

    int array[4];
    for (unsigned i = 0; i < sizeof(array) / sizeof(array[0]); ++i)
        for (unsigned j = 0; j < sizeof(array) / sizeof(array[0]); ++j)
            CU_ASSERT_EQUAL(cmp_sign(dict_ptr_cmp(&array[i], &array[j])),
                            (int)(i > j) - (int)(i < j));
    CU_ASSERT_EQUAL(dict_ptr_cmp(NULL, NULL), 0);

    /* dict_str_cmp() must agree in sign with strcmp(), which compares bytes as
     * unsigned char -- so a high byte sorts above any ASCII character. */
    static const char *const strings[] = {
        "", "a", "aa", "ab", "b", "A", "Z", "z", "\x7f", "\xC3\xA9", "\xC3", "\xff",
    };
    for (unsigned i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
        for (unsigned j = 0; j < sizeof(strings) / sizeof(strings[0]); ++j) {
            CU_ASSERT_EQUAL(cmp_sign(dict_str_cmp(strings[i], strings[j])),
                            cmp_sign(strcmp(strings[i], strings[j])));
        }
    }
    CU_ASSERT_TRUE(dict_str_cmp("\xC3\xA9", "a") > 0);
    CU_ASSERT_TRUE(dict_str_cmp("a", "\xC3\xA9") < 0);

    /* dict_str_hash() must be deterministic, and should differ for distinct
     * short strings. */
    for (unsigned i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i)
        CU_ASSERT_EQUAL(dict_str_hash(strings[i]), dict_str_hash(strings[i]));
    static const char *const hash_strings[] = { "a", "b", "c", "aa", "ab", "abc", "" };
    for (unsigned i = 0; i < sizeof(hash_strings) / sizeof(hash_strings[0]); ++i)
        for (unsigned j = i + 1; j < sizeof(hash_strings) / sizeof(hash_strings[0]); ++j)
            CU_ASSERT_NOT_EQUAL(dict_str_hash(hash_strings[i]),
                                dict_str_hash(hash_strings[j]));
}

void test_primes_geq()
{
    CU_ASSERT_TRUE(is_prime(2));
    CU_ASSERT_TRUE(is_prime(3));
    CU_ASSERT_FALSE(is_prime(4));
    CU_ASSERT_TRUE(is_prime(5));
    CU_ASSERT_FALSE(is_prime(6));
    CU_ASSERT_TRUE(is_prime(7));

    unsigned value = 0;
    do {
        const unsigned prime_geq_value = dict_prime_geq(value+1);
        CU_ASSERT_TRUE(prime_geq_value >= value+1);
        CU_ASSERT_TRUE(is_prime(prime_geq_value));
        CU_ASSERT_TRUE(dict_prime_geq(prime_geq_value) == prime_geq_value);
        value = prime_geq_value;
    } while (value != 4294967291U);
}

void test_version_string()
{
    char version_string[32];
    snprintf(version_string, sizeof(version_string), "%d.%d.%d",
         DICT_VERSION_MAJOR, DICT_VERSION_MINOR, DICT_VERSION_PATCH);
    CU_ASSERT_STRING_EQUAL(kDictVersionString, version_string);
}
