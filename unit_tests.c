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
#include "src/tree_common.h"        /* For tree_base, tree_node_base */

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
void test_rb_tree_verify_black_height(void);
void test_rb_tree_verify_global_order(void);
void test_rb_tree_verify_count(void);
void test_rb_tree_clear_rotation_count(void);
void test_hb_tree_traverse(void);
void test_hb_itor_search_exact(void);
void test_hb_tree_verify_global_order(void);
void test_skiplist_max_link_below_two(void);
void test_skiplist_verify_order(void);
void test_traverse_all_containers(void);
void test_select_null_out_params(void);
void test_wb_tree_verify_global_order(void);
void test_wb_tree_balance_above_6m_nodes(void);
void test_degenerate_tree_verify_and_path_lengths(void);
void test_sp_tr_tree_verify_order_and_count(void);
void test_dict_ptr_cmp_unrelated_objects(void);
void test_pr_tree_verify_order_and_count(void);
void test_hashtable_itor_resize_invalidates(void);
void test_hashtable_insert_existing_key_no_resize(void);
void test_hashtable_itor_clear_invalidates(void);
void test_hashtable2_itor_remove_invalidates(void);
void test_itor_survives_other_insert_and_remove(void);
void test_basic_skiplist(void);
void test_basic_splay_tree(void);
void test_basic_treap(void);
void test_basic_weight_balanced_tree(void);
void test_search(dict *dct, dict_itor *itor, const char *key, const char *value);
void test_closest_lookup(dict *dct, unsigned nkeys, bool keys_sorted);
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
    TEST_FUNC(test_rb_tree_verify_black_height),
    TEST_FUNC(test_rb_tree_verify_global_order),
    TEST_FUNC(test_rb_tree_verify_count),
    TEST_FUNC(test_rb_tree_clear_rotation_count),
    TEST_FUNC(test_hb_tree_traverse),
    TEST_FUNC(test_hb_itor_search_exact),
    TEST_FUNC(test_hb_tree_verify_global_order),
    TEST_FUNC(test_skiplist_max_link_below_two),
    TEST_FUNC(test_skiplist_verify_order),
    TEST_FUNC(test_traverse_all_containers),
    TEST_FUNC(test_select_null_out_params),
    TEST_FUNC(test_wb_tree_verify_global_order),
    TEST_FUNC(test_wb_tree_balance_above_6m_nodes),
    TEST_FUNC(test_degenerate_tree_verify_and_path_lengths),
    TEST_FUNC(test_sp_tr_tree_verify_order_and_count),
    TEST_FUNC(test_dict_ptr_cmp_unrelated_objects),
    TEST_FUNC(test_pr_tree_verify_order_and_count),
    TEST_FUNC(test_hashtable_itor_resize_invalidates),
    TEST_FUNC(test_hashtable_insert_existing_key_no_resize),
    TEST_FUNC(test_hashtable_itor_clear_invalidates),
    TEST_FUNC(test_hashtable2_itor_remove_invalidates),
    TEST_FUNC(test_itor_survives_other_insert_and_remove),
    TEST_FUNC(test_basic_skiplist),
    TEST_FUNC(test_basic_splay_tree),
    TEST_FUNC(test_basic_treap),
    TEST_FUNC(test_basic_weight_balanced_tree),
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
    assert(p);
    bytes_malloced += n;
    p[0] = n;
    return &p[1];
}

static void
custom_free(void* p)
{
    assert(p);
    size_t* sp = p;
    bytes_malloced -= sp[-1];
    free(sp - 1);
}

int
main()
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

void
test_closest_lookup(dict *dct, unsigned nkeys, bool keys_sorted)
{
    if (dict_is_sorted(dct) && keys_sorted) {
        for (unsigned i = 0; i < nkeys; ++i) {
            const void* key = NULL;
            void* datum = NULL;
            if (dct->_vtable->select) {
                CU_ASSERT_TRUE(dict_select(dct, i, &key, &datum));
                CU_ASSERT_EQUAL(key, sorted_keys[i].key);
                CU_ASSERT_EQUAL(datum, sorted_keys[i].value);
            }
        }
        const void* key = NULL;
        void* datum = NULL;
        CU_ASSERT_FALSE(dict_select(dct, nkeys, &key, &datum));
        CU_ASSERT_EQUAL(key, (const void*)NULL);
        CU_ASSERT_PTR_NULL(datum);
    }

    dict_itor* itor = dict_itor_new(dct);
    for (unsigned i = 0; i < NUM_CLOSEST_LOOKUP_INFOS; i++) {
        const struct closest_lookup_info* const info = &closest_lookup_infos[i];
        if (!dict_is_sorted(dct)) {
            CU_ASSERT_PTR_NULL(dct->_vtable->search_le);
            CU_ASSERT_PTR_NULL(dct->_vtable->search_lt);
            CU_ASSERT_PTR_NULL(dct->_vtable->search_ge);
            CU_ASSERT_PTR_NULL(dct->_vtable->search_gt);
            CU_ASSERT_PTR_NULL(itor->_vtable->search_le);
            CU_ASSERT_PTR_NULL(itor->_vtable->search_lt);
            CU_ASSERT_PTR_NULL(itor->_vtable->search_ge);
            CU_ASSERT_PTR_NULL(itor->_vtable->search_gt);
            const void* key = NULL;
            void* datum = NULL;
            CU_ASSERT_FALSE(dict_select(dct, i, &key, &datum));
            CU_ASSERT_EQUAL(key, (const void*)NULL);
            CU_ASSERT_PTR_NULL(datum);
            CU_ASSERT_PTR_NULL(dict_search_le(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_lt(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_ge(dct, info->key));
            CU_ASSERT_PTR_NULL(dict_search_gt(dct, info->key));
            continue;
        }
        if (nkeys < NUM_SORTED_KEYS)
            continue;
        if (info->le_key) {
            if (dct->_vtable->search_le) {
                CU_ASSERT_PTR_NOT_NULL(dict_search_le(dct, info->key));
                void **datum = dict_search_le(dct, info->key);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->le_val) == 0);
            }
            if (itor->_vtable->search_le) {
                CU_ASSERT_TRUE(dict_itor_search_le(itor, info->key));
                CU_ASSERT_TRUE(dict_itor_valid(itor));
                CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), info->le_key);
                void **datum = dict_itor_datum(itor);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->le_val) == 0);
            }
        } else {
            CU_ASSERT_PTR_NULL(dict_search_le(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_le(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (info->lt_key) {
            if (dct->_vtable->search_lt) {
                CU_ASSERT_PTR_NOT_NULL(dict_search_lt(dct, info->key));
                void** datum = dict_search_lt(dct, info->key);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->lt_val) == 0);
            }
            if (itor->_vtable->search_lt) {
                CU_ASSERT_TRUE(dict_itor_search_lt(itor, info->key));
                CU_ASSERT_TRUE(dict_itor_valid(itor));
                CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), info->lt_key);
                void** datum = dict_itor_datum(itor);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->lt_val) == 0);
            }
        } else {
            CU_ASSERT_PTR_NULL(dict_search_lt(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_lt(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (info->ge_key) {
            if (dct->_vtable->search_ge) {
                CU_ASSERT_PTR_NOT_NULL(dict_search_ge(dct, info->key));
                void** datum = dict_search_ge(dct, info->key);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->ge_val) == 0);
            }
            if (itor->_vtable->search_ge) {
                CU_ASSERT_TRUE(dict_itor_search_ge(itor, info->key));
                CU_ASSERT_TRUE(dict_itor_valid(itor));
                CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), info->ge_key);
                void** datum = dict_itor_datum(itor);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->ge_val) == 0);
            }
        } else {
            CU_ASSERT_PTR_NULL(dict_search_ge(dct, info->key));
            CU_ASSERT_FALSE(dict_itor_search_ge(itor, info->key));
            CU_ASSERT_FALSE(dict_itor_valid(itor));
            CU_ASSERT_PTR_NULL(dict_itor_key(itor));
            CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
        }
        if (info->gt_key) {
            if (dct->_vtable->search_gt) {
                CU_ASSERT_PTR_NOT_NULL(dict_search_gt(dct, info->key));
                void** datum = dict_search_gt(dct, info->key);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->gt_val) == 0);
            }
            if (itor->_vtable->search_gt) {
                CU_ASSERT_TRUE(dict_itor_search_gt(itor, info->key));
                CU_ASSERT_TRUE(dict_itor_valid(itor));
                CU_ASSERT_STRING_EQUAL(dict_itor_key(itor), info->gt_key);
                void** datum = dict_itor_datum(itor);
                CU_ASSERT_TRUE(datum != NULL && *datum != NULL && strcmp(*datum, info->gt_val) == 0);
            }
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

    char *last_key = NULL;
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

    test_closest_lookup(dct, nkeys, keys_sorted);

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

/* The tests below corrupt an rb_tree through the generic tree_base and
 * tree_node_base views, whose layout rb_tree.c static_asserts it matches. The
 * |parent| member of an rb_node holds the colour in its low bit (1 = black). */
#define RB_NODE(n)          ((tree_node_base*)(n))
#define RB_FLIP_COLOR(n)    (RB_NODE(n)->parent = (tree_node_base*)((intptr_t)RB_NODE(n)->parent ^ 1))

void test_rb_tree_verify_black_height()
{
    int keys[] = { 10, 5 };
    rb_tree* tree = rb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 2; ++i)
        CU_ASSERT_TRUE(rb_tree_insert(tree, &keys[i]).inserted);
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    /* Root 10 (black) with only a red left child 5. Turning 5 black gives the
     * path 10->NIL one black node and 10->5->NIL two. */
    tree_node_base* root = ((tree_base*)tree)->root;
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink);
    CU_ASSERT_PTR_NULL(root->rlink);
    RB_FLIP_COLOR(root->llink);
    CU_ASSERT_FALSE(rb_tree_verify(tree));
    RB_FLIP_COLOR(root->llink);
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    CU_ASSERT_EQUAL(rb_tree_free(tree, NULL), 2);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_rb_tree_verify_global_order()
{
    int keys[] = { 10, 5, 15, 3, 7 };
    rb_tree* tree = rb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 5; ++i)
        CU_ASSERT_TRUE(rb_tree_insert(tree, &keys[i]).inserted);
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    /* Shape is 10 -> (5 -> (3, 7), 15). Changing 7 to 12 keeps it ordered
     * relative to its parent 5, but puts 12 in the left subtree of 10. */
    tree_node_base* root = ((tree_base*)tree)->root;
    CU_ASSERT_EQUAL_FATAL(*(int*)root->key, 10);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink->rlink);
    CU_ASSERT_PTR_EQUAL_FATAL(root->llink->rlink->key, &keys[4]);
    keys[4] = 12;
    CU_ASSERT_FALSE(rb_tree_verify(tree));
    keys[4] = 7;
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    CU_ASSERT_EQUAL(rb_tree_free(tree, NULL), 5);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_rb_tree_verify_count()
{
    int keys[] = { 1, 2, 3 };
    rb_tree* tree = rb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 3; ++i)
        CU_ASSERT_TRUE(rb_tree_insert(tree, &keys[i]).inserted);
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    ((tree_base*)tree)->count++;
    CU_ASSERT_FALSE(rb_tree_verify(tree));
    ((tree_base*)tree)->count--;
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    CU_ASSERT_EQUAL(rb_tree_free(tree, NULL), 3);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_rb_tree_clear_rotation_count()
{
    int keys[] = { 1, 2, 3, 4, 5 };
    rb_tree* tree = rb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 5; ++i)
        CU_ASSERT_TRUE(rb_tree_insert(tree, &keys[i]).inserted);
    /* Ascending inserts always rotate. */
    CU_ASSERT_NOT_EQUAL(((tree_base*)tree)->rotation_count, 0);

    CU_ASSERT_EQUAL(rb_tree_clear(tree, NULL), 5);
    CU_ASSERT_EQUAL(((tree_base*)tree)->rotation_count, 0);
    CU_ASSERT_TRUE(rb_tree_verify(tree));

    CU_ASSERT_EQUAL(rb_tree_free(tree, NULL), 0);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

struct traverse_state {
    int         keys[16];
    unsigned    count;
    unsigned    stop_after;
};

static bool
traverse_visit(const void* key, void* datum, void* user_data)
{
    (void)datum;
    struct traverse_state* state = user_data;
    if (state->count < 16)
        state->keys[state->count] = *(const int*)key;
    return ++state->count != state->stop_after;
}

void test_hb_tree_traverse()
{
    int keys[10];
    hb_tree* tree = hb_tree_new(dict_int_cmp);
    struct traverse_state state = { .count = 0, .stop_after = 0 };
    CU_ASSERT_EQUAL(hb_tree_traverse(tree, traverse_visit, &state), 0);

    /* Ascending inserts rotate, leaving balance bits set in the parent
     * pointers that the generic tree_traverse() must not decode. */
    for (int i = 0; i < 10; ++i) {
        keys[i] = i;
        CU_ASSERT_TRUE(hb_tree_insert(tree, &keys[i]).inserted);
    }
    CU_ASSERT_EQUAL(hb_tree_traverse(tree, traverse_visit, &state), 10);
    CU_ASSERT_EQUAL(state.count, 10);
    for (int i = 0; i < 10; ++i)
        CU_ASSERT_EQUAL(state.keys[i], i);

    /* A visit function returning false stops the traversal. */
    state = (struct traverse_state) { .count = 0, .stop_after = 3 };
    CU_ASSERT_EQUAL(hb_tree_traverse(tree, traverse_visit, &state), 3);

    CU_ASSERT_EQUAL(hb_tree_free(tree, NULL), 10);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_hb_itor_search_exact()
{
    int keys[] = { 10, 20 };
    int probe = 15;
    hb_tree* tree = hb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 2; ++i)
        CU_ASSERT_TRUE(hb_tree_insert(tree, &keys[i]).inserted);

    hb_itor* itor = hb_itor_new(tree);
    CU_ASSERT_TRUE(hb_itor_search(itor, &keys[1]));
    CU_ASSERT_PTR_EQUAL(hb_itor_key(itor), &keys[1]);
    /* 15 is absent: an exact search must fail, not land on 20. */
    CU_ASSERT_FALSE(hb_itor_search(itor, &probe));
    CU_ASSERT_FALSE(hb_itor_valid(itor));
    CU_ASSERT_TRUE(hb_itor_search_ge(itor, &probe));
    CU_ASSERT_PTR_EQUAL(hb_itor_key(itor), &keys[1]);
    hb_itor_free(itor);

    CU_ASSERT_EQUAL(hb_tree_free(tree, NULL), 2);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_hb_tree_verify_global_order()
{
    int keys[] = { 10, 5, 15, 3, 7 };
    hb_tree* tree = hb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 5; ++i)
        CU_ASSERT_TRUE(hb_tree_insert(tree, &keys[i]).inserted);
    CU_ASSERT_TRUE(hb_tree_verify(tree));

    /* Shape is 10 -> (5 -> (3, 7), 15). Changing 7 to 12 keeps it ordered
     * relative to its parent 5, but puts 12 in the left subtree of 10. */
    tree_node_base* root = ((tree_base*)tree)->root;
    CU_ASSERT_EQUAL_FATAL(*(int*)root->key, 10);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink->rlink);
    CU_ASSERT_PTR_EQUAL_FATAL(root->llink->rlink->key, &keys[4]);
    keys[4] = 12;
    CU_ASSERT_FALSE(hb_tree_verify(tree));
    keys[4] = 7;
    CU_ASSERT_TRUE(hb_tree_verify(tree));

    CU_ASSERT_EQUAL(hb_tree_free(tree, NULL), 5);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

void test_skiplist_max_link_below_two()
{
    /* A node gets at most max_link - 1 links, so max_link 0 or 1 used to give
     * nodes that were never linked in: every element was lost and leaked. */
    for (unsigned max_link = 0; max_link < 2; ++max_link) {
        int keys[100];
        dict* dct = skiplist_dict_new(dict_int_cmp, max_link);
        CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
        for (int i = 0; i < 100; ++i) {
            keys[i] = i;
            CU_ASSERT_TRUE(dict_insert(dct, &keys[i]).inserted);
        }
        CU_ASSERT_TRUE(dict_verify(dct));
        CU_ASSERT_EQUAL(dict_count(dct), 100);
        for (int i = 0; i < 100; ++i)
            CU_ASSERT_PTR_NOT_NULL(dict_search(dct, &keys[i]));
        CU_ASSERT_TRUE(dict_remove(dct, &keys[50]).removed);
        CU_ASSERT_TRUE(dict_verify(dct));
        CU_ASSERT_EQUAL(dict_free(dct, NULL), 99);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_skiplist_verify_order()
{
    int keys[] = { 1, 2, 3, 4, 5 };
    skiplist* list = skiplist_new(dict_int_cmp, 13);
    for (unsigned i = 0; i < 5; ++i)
        CU_ASSERT_TRUE(skiplist_insert(list, &keys[i]).inserted);
    CU_ASSERT_TRUE(skiplist_verify(list));

    /* 1 2 10 4 5 is out of order, whatever links each node has. */
    keys[2] = 10;
    CU_ASSERT_FALSE(skiplist_verify(list));
    keys[2] = 3;
    CU_ASSERT_TRUE(skiplist_verify(list));

    CU_ASSERT_EQUAL(skiplist_free(list, NULL), 5);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

#define TRAVERSE_NKEYS 40

struct traverse_all_state {
    const void* seen[TRAVERSE_NKEYS];
    unsigned    count;
    unsigned    stop_after;
};

static bool
traverse_all_visit(const void* key, void* datum, void* user_data)
{
    struct traverse_all_state* state = user_data;
    /* Each key was inserted with itself as its datum. */
    CU_ASSERT_PTR_EQUAL(datum, key);
    if (state->count < TRAVERSE_NKEYS)
        state->seen[state->count] = key;
    return ++state->count != state->stop_after;
}

static dict* new_hashtable_1(void)  { return hashtable_dict_new(dict_str_cmp, dict_str_hash, 1); }
static dict* new_hashtable_n(void)  { return hashtable_dict_new(dict_str_cmp, dict_str_hash, 97); }
static dict* new_hashtable2_1(void) { return hashtable2_dict_new(dict_str_cmp, dict_str_hash, 1); }
static dict* new_hashtable2_n(void) { return hashtable2_dict_new(dict_str_cmp, dict_str_hash, 97); }
static dict* new_hb_tree(void)      { return hb_dict_new(dict_str_cmp); }
static dict* new_pr_tree(void)      { return pr_dict_new(dict_str_cmp); }
static dict* new_rb_tree(void)      { return rb_dict_new(dict_str_cmp); }
static dict* new_skiplist(void)     { return skiplist_dict_new(dict_str_cmp, 13); }
static dict* new_sp_tree(void)      { return sp_dict_new(dict_str_cmp); }
static dict* new_tr_tree(void)      { return tr_dict_new(dict_str_cmp, NULL); }
static dict* new_wb_tree(void)      { return wb_dict_new(dict_str_cmp); }

/* Every container, for tests of code paths they all implement. */
static const struct {
    const char* name;
    dict*       (*new_dict)(void);
} containers[] = {
    { "hashtable/1", new_hashtable_1 },   { "hashtable/97", new_hashtable_n },
    { "hashtable2/1", new_hashtable2_1 }, { "hashtable2/97", new_hashtable2_n },
    { "hb_tree", new_hb_tree },           { "pr_tree", new_pr_tree },
    { "rb_tree", new_rb_tree },           { "skiplist", new_skiplist },
    { "sp_tree", new_sp_tree },           { "tr_tree", new_tr_tree },
    { "wb_tree", new_wb_tree },
};
#define NUM_CONTAINERS (sizeof(containers) / sizeof(containers[0]))

void test_traverse_all_containers()
{
    /* "k00" .. "k39": ascending in dict_str_cmp order. */
    char keys[TRAVERSE_NKEYS][4];
    for (unsigned i = 0; i < TRAVERSE_NKEYS; ++i)
        snprintf(keys[i], sizeof(keys[i]), "k%02u", i);

    for (size_t c = 0; c < NUM_CONTAINERS; ++c) {
        dict* dct = containers[c].new_dict();
        CU_ASSERT_PTR_NOT_NULL_FATAL(dct);

        struct traverse_all_state state = { .count = 0, .stop_after = 0 };
        CU_ASSERT_EQUAL(dict_traverse(dct, traverse_all_visit, &state), 0);
        CU_ASSERT_EQUAL(state.count, 0);

        /* Insert in an order that is neither ascending nor descending, so the
         * trees rotate and the hash tables chain. */
        for (unsigned i = 0; i < TRAVERSE_NKEYS; ++i) {
            char* key = keys[(i * 7) % TRAVERSE_NKEYS];
            dict_insert_result result = dict_insert(dct, key);
            CU_ASSERT_TRUE(result.inserted);
            *result.datum_ptr = key;
        }
        CU_ASSERT_TRUE(dict_verify(dct));

        state = (struct traverse_all_state) { .count = 0, .stop_after = 0 };
        CU_ASSERT_EQUAL(dict_traverse(dct, traverse_all_visit, &state), TRAVERSE_NKEYS);
        CU_ASSERT_EQUAL(state.count, TRAVERSE_NKEYS);
        for (unsigned i = 0; i < TRAVERSE_NKEYS; ++i) {
            /* Every key exactly once. */
            unsigned found = 0;
            for (unsigned j = 0; j < TRAVERSE_NKEYS; ++j)
                found += (state.seen[j] == keys[i]);
            if (found != 1)
                fprintf(stderr, "%s: key %s visited %u times\n",
                        containers[c].name, keys[i], found);
            CU_ASSERT_EQUAL(found, 1);
            /* Sorted containers visit in key order. */
            if (dict_is_sorted(dct))
                CU_ASSERT_PTR_EQUAL(state.seen[i], keys[i]);
        }

        /* Returning false from the visit function stops the traversal. */
        state = (struct traverse_all_state) { .count = 0, .stop_after = 5 };
        CU_ASSERT_EQUAL(dict_traverse(dct, traverse_all_visit, &state), 5);
        CU_ASSERT_EQUAL(state.count, 5);

        CU_ASSERT_TRUE(dict_verify(dct));
        CU_ASSERT_EQUAL(dict_free(dct, NULL), TRAVERSE_NKEYS);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_select_null_out_params()
{
    /* "k00" .. "k09": ascending in dict_str_cmp order. */
    char keys[10][4];
    for (unsigned i = 0; i < 10; ++i)
        snprintf(keys[i], sizeof(keys[i]), "k%02u", i);

    for (size_t c = 0; c < NUM_CONTAINERS; ++c) {
        dict* dct = containers[c].new_dict();
        CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
        /* Containers without select make dict_select() evaluate to false. */
        const bool has_select = dct->_vtable->select != NULL;

        /* Either out-parameter may be NULL, also when |n| is out of range. */
        const void* key = keys[0];
        void* datum = keys[0];
        CU_ASSERT_FALSE(dict_select(dct, 0, NULL, NULL));
        CU_ASSERT_FALSE(dict_select(dct, 0, NULL, &datum));
        CU_ASSERT_FALSE(dict_select(dct, 0, &key, NULL));
        if (has_select) {
            CU_ASSERT_PTR_NULL(datum);
            CU_ASSERT_PTR_NULL(key);
        }

        for (unsigned i = 0; i < 10; ++i) {
            char* k = keys[(i * 3) % 10];
            dict_insert_result result = dict_insert(dct, k);
            CU_ASSERT_TRUE(result.inserted);
            *result.datum_ptr = k;
        }
        CU_ASSERT_TRUE(dict_verify(dct));

        for (unsigned n = 0; n < 10; ++n) {
            key = NULL;
            datum = NULL;
            CU_ASSERT_EQUAL(dict_select(dct, n, NULL, &datum), has_select);
            CU_ASSERT_EQUAL(dict_select(dct, n, &key, NULL), has_select);
            CU_ASSERT_EQUAL(dict_select(dct, n, NULL, NULL), has_select);
            if (has_select) {
                CU_ASSERT_PTR_EQUAL(datum, keys[n]);
                CU_ASSERT_PTR_EQUAL(key, keys[n]);
            }
        }
        datum = keys[0];
        CU_ASSERT_FALSE(dict_select(dct, 10, NULL, &datum));
        if (has_select)
            CU_ASSERT_PTR_NULL(datum);

        CU_ASSERT_TRUE(dict_verify(dct));
        CU_ASSERT_EQUAL(dict_free(dct, NULL), 10);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_wb_tree_verify_global_order()
{
    int keys[] = { 10, 5, 15, 3, 7 };
    wb_tree* tree = wb_tree_new(dict_int_cmp);
    for (unsigned i = 0; i < 5; ++i)
        CU_ASSERT_TRUE(wb_tree_insert(tree, &keys[i]).inserted);
    CU_ASSERT_TRUE(wb_tree_verify(tree));

    /* Shape is 10 -> (5 -> (3, 7), 15). Changing 7 to 12 keeps it ordered
     * relative to its parent 5, but puts 12 in the left subtree of 10. */
    tree_node_base* root = ((tree_base*)tree)->root;
    CU_ASSERT_EQUAL_FATAL(*(int*)root->key, 10);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink);
    CU_ASSERT_PTR_NOT_NULL_FATAL(root->llink->rlink);
    CU_ASSERT_PTR_EQUAL_FATAL(root->llink->rlink->key, &keys[4]);
    keys[4] = 12;
    CU_ASSERT_FALSE(wb_tree_verify(tree));
    keys[4] = 7;
    CU_ASSERT_TRUE(wb_tree_verify(tree));

    CU_ASSERT_EQUAL(wb_tree_free(tree, NULL), 5);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

/* Compares keys that are integers stored in the pointer itself. */
static int
uintptr_key_cmp(const void* k1, const void* k2)
{
    const uintptr_t a = (uintptr_t) k1;
    const uintptr_t b = (uintptr_t) k2;
    return (a > b) - (a < b);
}

/* Slow and memory hungry (~2 s and ~400 MB natively), so it only runs when
 * LIBDICT_SLOW_TESTS=1 is set in the environment. */
void test_wb_tree_balance_above_6m_nodes()
{
    const char* slow = getenv("LIBDICT_SLOW_TESTS");
    if (!slow || strcmp(slow, "1") != 0) {
        fprintf(stderr, "test_wb_tree_balance_above_6m_nodes: skipped, "
                "set LIBDICT_SLOW_TESTS=1 to run it\n");
        return;
    }

    /* The balance tests multiply 32-bit weights by up to 1000, which wrapped
     * once a subtree held more than ~6,075,626 nodes (weight * 707). The
     * spurious rotations that followed unbalanced the tree. */
    const uintptr_t nkeys = 6200000;
    wb_tree* tree = wb_tree_new(uintptr_key_cmp);
    for (uintptr_t i = 1; i <= nkeys; ++i) {
        if (!wb_tree_insert(tree, (void*) i).inserted) {
            CU_FAIL("wb_tree_insert failed");
            break;
        }
    }
    CU_ASSERT_EQUAL(wb_tree_count(tree), nkeys);
    const bool verified = wb_tree_verify(tree);
    CU_ASSERT_TRUE(verified);
    /* BB[alpha] with alpha = 0.293 has height at most
     * log(n + 1) / log(1 / (1 - alpha)), which is 45.1 here. Only measured on
     * a verified tree, since the recursive path-length helper could exhaust
     * the stack on a degenerate one. */
    if (verified)
        CU_ASSERT_TRUE(wb_tree_max_path_length(tree) <= 46);

    CU_ASSERT_EQUAL(wb_tree_free(tree, NULL), nkeys);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

/* Priority function for a treap: a larger key sits closer to the root. */
static unsigned
uintptr_key_prio(const void* key)
{
    return (unsigned) (uintptr_t) key;
}

#define DEGENERATE_NKEYS 1000000

/* Checks the verifier and the path-length statistics on a tree that is one
 * chain of DEGENERATE_NKEYS nodes. Recursive implementations exhaust the stack
 * on such a tree. */
static void
check_degenerate_tree(const char* name, bool verified, size_t min_path,
                      size_t max_path, size_t total_path)
{
    /* Every node but the deepest has exactly one child, so a link is already
     * absent at the root. Sum in size_t, wrapping where size_t is 32 bits
     * exactly as the implementation does. */
    size_t expected_total = 0;
    for (size_t depth = 1; depth <= DEGENERATE_NKEYS; ++depth)
        expected_total += depth;
    if (!verified || min_path != 1 || max_path != DEGENERATE_NKEYS ||
        total_path != expected_total)
        fprintf(stderr, "%s: verified=%d min=%zu max=%zu total=%zu\n",
                name, verified, min_path, max_path, total_path);
    CU_ASSERT_TRUE(verified);
    CU_ASSERT_EQUAL(min_path, 1);
    CU_ASSERT_EQUAL(max_path, DEGENERATE_NKEYS);
    CU_ASSERT_EQUAL(total_path, expected_total);
}

void test_degenerate_tree_verify_and_path_lengths()
{
    /* Ascending inserts splay each new key to the root, leaving the previous
     * tree as its left subtree: a left chain, built in O(1) per insert. */
    sp_tree* sp = sp_tree_new(uintptr_key_cmp);
    for (uintptr_t i = 1; i <= DEGENERATE_NKEYS; ++i)
        CU_ASSERT_TRUE(sp_tree_insert(sp, (void*) i).inserted);
    check_degenerate_tree("sp_tree", sp_tree_verify(sp),
                          sp_tree_min_path_length(sp),
                          sp_tree_max_path_length(sp),
                          sp_tree_total_path_length(sp));
    CU_ASSERT_EQUAL(sp_tree_free(sp, NULL), DEGENERATE_NKEYS);
    CU_ASSERT_EQUAL(bytes_malloced, 0);

    /* With priority equal to the key, each new (largest) key rotates up to
     * the root: the same left chain. */
    tr_tree* tr = tr_tree_new(uintptr_key_cmp, uintptr_key_prio);
    for (uintptr_t i = 1; i <= DEGENERATE_NKEYS; ++i)
        CU_ASSERT_TRUE(tr_tree_insert(tr, (void*) i).inserted);
    check_degenerate_tree("tr_tree", tr_tree_verify(tr),
                          tr_tree_min_path_length(tr),
                          tr_tree_max_path_length(tr),
                          tr_tree_total_path_length(tr));
    CU_ASSERT_EQUAL(tr_tree_free(tr, NULL), DEGENERATE_NKEYS);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

/* Corrupts |dct|, a tree with int keys, in two ways that only a global check
 * catches, and checks that dict_verify() rejects each and accepts the repair. */
static void
check_verify_order_and_count(const char* name, dict* dct)
{
    tree_base* tree = dict_private(dct);
    CU_ASSERT_TRUE_FATAL(dict_verify(dct));

    /* The rightmost node of the root's left subtree, if it is a right child,
     * is ordered against its parent even when its key exceeds the root's. */
    tree_node_base* node = tree->root->llink;
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node->rlink);
    while (node->rlink)
        node = node->rlink;
    int* key = node->key;
    const int saved = *key;
    *key = *(int*)tree->root->key + 1;
    if (dict_verify(dct))
        fprintf(stderr, "%s: accepted a key on the wrong side of the root\n", name);
    CU_ASSERT_FALSE(dict_verify(dct));
    *key = saved;
    CU_ASSERT_TRUE(dict_verify(dct));

    tree->count++;
    CU_ASSERT_FALSE(dict_verify(dct));
    tree->count--;
    CU_ASSERT_TRUE(dict_verify(dct));
}

/* Treap priorities giving the shape 40 -> (20 -> (10, 30), 60 -> (50, 70)). */
static unsigned
shape_prio(const void* key)
{
    switch (*(const int*)key) {
    case 40: return 3;
    case 20: case 60: return 2;
    default: return 1;
    }
}

void test_sp_tr_tree_verify_order_and_count()
{
    int keys[] = { 10, 20, 30, 40, 50, 60, 70 };

    /* Searching 20 and then 40 splays them to the root in turn, leaving
     * 40 -> (20 -> (10, 30), ...). */
    dict* sp = sp_dict_new(dict_int_cmp);
    for (unsigned i = 0; i < 7; ++i)
        CU_ASSERT_TRUE(dict_insert(sp, &keys[i]).inserted);
    CU_ASSERT_PTR_NOT_NULL(dict_search(sp, &keys[1]));
    CU_ASSERT_PTR_NOT_NULL(dict_search(sp, &keys[3]));
    check_verify_order_and_count("sp_tree", sp);
    CU_ASSERT_EQUAL(dict_free(sp, NULL), 7);

    dict* tr = tr_dict_new(dict_int_cmp, shape_prio);
    for (unsigned i = 0; i < 7; ++i)
        CU_ASSERT_TRUE(dict_insert(tr, &keys[i]).inserted);
    check_verify_order_and_count("tr_tree", tr);
    CU_ASSERT_EQUAL(dict_free(tr, NULL), 7);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

/* dict_ptr_cmp is used on keys that are pointers to separately allocated
 * objects, where relational operators on the pointers themselves are
 * undefined (C11 6.5.8p5). The results are the same either way on common
 * targets, so the undefined comparison itself is only caught by building with
 * -fsanitize=address,pointer-compare and running with
 * ASAN_OPTIONS=detect_invalid_pointer_pairs=2. */
void test_dict_ptr_cmp_unrelated_objects()
{
    int* a = malloc(sizeof(*a));
    int* b = malloc(sizeof(*b));
    CU_ASSERT_PTR_NOT_NULL_FATAL(a);
    CU_ASSERT_PTR_NOT_NULL_FATAL(b);
    const int expected = ((uintptr_t) a > (uintptr_t) b) - ((uintptr_t) a < (uintptr_t) b);
    CU_ASSERT_NOT_EQUAL(expected, 0);
    CU_ASSERT_EQUAL(dict_ptr_cmp(a, b), expected);
    CU_ASSERT_EQUAL(dict_ptr_cmp(b, a), -expected);
    CU_ASSERT_EQUAL(dict_ptr_cmp(a, a), 0);
    CU_ASSERT_EQUAL(dict_ptr_cmp(NULL, NULL), 0);
    free(a);
    free(b);
}

void test_pr_tree_verify_order_and_count()
{
    int keys[] = { 10, 20, 30, 40, 50, 60, 70 };
    dict* pr = pr_dict_new(dict_int_cmp);
    for (unsigned i = 0; i < 7; ++i)
        CU_ASSERT_TRUE(dict_insert(pr, &keys[i]).inserted);
    check_verify_order_and_count("pr_tree", pr);
    CU_ASSERT_EQUAL(dict_free(pr, NULL), 7);
    CU_ASSERT_EQUAL(bytes_malloced, 0);
}

#define HASH_NKEYS 64

/* "k00" .. "k63", for the hash table iterator tests. */
static char hash_keys[HASH_NKEYS][4];

static void
init_hash_keys(void)
{
    for (unsigned i = 0; i < HASH_NKEYS; ++i)
        snprintf(hash_keys[i], sizeof(hash_keys[i]), "k%02u", i);
}

/* Number of slots of a hash table dict, which changes when it resizes. */
static size_t
hash_dict_size(dict* dct)
{
    return is_hashtable(dct) ? hashtable_size(dict_private(dct))
                             : hashtable2_size(dict_private(dct));
}

/* An iterator that was invalidated by a modification must behave exactly like
 * an unpositioned one. */
static void
check_itor_invalid(dict_itor* itor)
{
    CU_ASSERT_FALSE(dict_itor_valid(itor));
    CU_ASSERT_PTR_NULL(dict_itor_key(itor));
    CU_ASSERT_PTR_NULL(dict_itor_datum(itor));
    CU_ASSERT_FALSE(dict_itor_next(itor));
    CU_ASSERT_FALSE(dict_itor_valid(itor));
    CU_ASSERT_FALSE(dict_itor_remove(itor));
}

static dict*
new_hash_dict(bool chained, unsigned size)
{
    return chained ? hashtable_dict_new(dict_str_cmp, dict_str_hash, size)
                   : hashtable2_dict_new(dict_str_cmp, dict_str_hash, size);
}

void test_hashtable_itor_resize_invalidates()
{
    init_hash_keys();
    for (int chained = 1; chained >= 0; --chained) {
        dict* dct = new_hash_dict(chained, 1);
        CU_ASSERT_TRUE(dict_insert(dct, hash_keys[0]).inserted);
        dict_itor* itor = dict_itor_new(dct);
        CU_ASSERT_TRUE(dict_itor_first(itor));

        /* Inserts that do not resize leave the iterator where it was; the
         * first one that does invalidates it, since it rehashes every
         * element into a different slot. */
        const size_t size = hash_dict_size(dct);
        bool resized = false;
        for (unsigned i = 1; i < HASH_NKEYS && !resized; ++i) {
            CU_ASSERT_TRUE(dict_insert(dct, hash_keys[i]).inserted);
            if (hash_dict_size(dct) != size) {
                resized = true;
                check_itor_invalid(itor);
            } else {
                CU_ASSERT_TRUE(dict_itor_valid(itor));
                CU_ASSERT_PTR_EQUAL(dict_itor_key(itor), hash_keys[0]);
            }
        }
        CU_ASSERT_TRUE(resized);
        CU_ASSERT_TRUE(dict_verify(dct));

        /* Repositioning makes it usable again. */
        CU_ASSERT_TRUE(dict_itor_first(itor));
        CU_ASSERT_TRUE(dict_itor_valid(itor));

        dict_itor_free(itor);
        dict_free(dct, NULL);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_hashtable_insert_existing_key_no_resize()
{
    init_hash_keys();
    for (int chained = 1; chained >= 0; --chained) {
        /* Find how many keys a table starting at size 1 holds before the next
         * new key makes it grow. */
        dict* probe = new_hash_dict(chained, 1);
        const size_t size = hash_dict_size(probe);
        unsigned fill = 0;
        while (fill < HASH_NKEYS) {
            CU_ASSERT_TRUE(dict_insert(probe, hash_keys[fill]).inserted);
            if (hash_dict_size(probe) != size)
                break;
            ++fill;
        }
        dict_free(probe, NULL);
        CU_ASSERT_TRUE_FATAL(fill > 0 && fill < HASH_NKEYS);

        /* Fill a fresh table to exactly that point: inserting a key that is
         * already present adds nothing, so it must not grow the table. */
        dict* dct = new_hash_dict(chained, 1);
        for (unsigned i = 0; i < fill; ++i)
            CU_ASSERT_TRUE(dict_insert(dct, hash_keys[i]).inserted);
        dict_itor* itor = dict_itor_new(dct);
        CU_ASSERT_TRUE(dict_itor_first(itor));
        const void* key = dict_itor_key(itor);

        CU_ASSERT_FALSE(dict_insert(dct, hash_keys[0]).inserted);
        CU_ASSERT_EQUAL(hash_dict_size(dct), size);
        CU_ASSERT_TRUE(dict_itor_valid(itor));
        CU_ASSERT_PTR_EQUAL(dict_itor_key(itor), key);
        CU_ASSERT_TRUE(dict_verify(dct));

        dict_itor_free(itor);
        dict_free(dct, NULL);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_hashtable_itor_clear_invalidates()
{
    init_hash_keys();
    for (int chained = 1; chained >= 0; --chained) {
        dict* dct = new_hash_dict(chained, 97);
        for (unsigned i = 0; i < 10; ++i)
            CU_ASSERT_TRUE(dict_insert(dct, hash_keys[i]).inserted);
        dict_itor* itor = dict_itor_new(dct);
        CU_ASSERT_TRUE(dict_itor_search(itor, hash_keys[3]));

        CU_ASSERT_EQUAL(dict_clear(dct, NULL), 10);
        check_itor_invalid(itor);

        CU_ASSERT_TRUE(dict_insert(dct, hash_keys[5]).inserted);
        CU_ASSERT_TRUE(dict_itor_first(itor));
        CU_ASSERT_PTR_EQUAL(dict_itor_key(itor), hash_keys[5]);
        CU_ASSERT_TRUE(dict_verify(dct));

        dict_itor_free(itor);
        dict_free(dct, NULL);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_hashtable2_itor_remove_invalidates()
{
    init_hash_keys();
    /* Removing from an open-addressing table moves other elements to fill
     * the gap, so any removal invalidates its iterators: the slot an iterator
     * holds may now be empty or hold a different key. */
    for (int own = 0; own <= 1; ++own) {
        dict* dct = new_hash_dict(false, 97);
        for (unsigned i = 0; i < 20; ++i)
            CU_ASSERT_TRUE(dict_insert(dct, hash_keys[i]).inserted);
        dict_itor* itor = dict_itor_new(dct);
        CU_ASSERT_TRUE(dict_itor_search(itor, hash_keys[7]));

        CU_ASSERT_TRUE(dict_remove(dct, hash_keys[own ? 7 : 12]).removed);
        CU_ASSERT_TRUE(dict_verify(dct));
        check_itor_invalid(itor);

        CU_ASSERT_TRUE(dict_itor_search(itor, hash_keys[own ? 8 : 7]));
        CU_ASSERT_TRUE(dict_itor_valid(itor));

        dict_itor_free(itor);
        dict_free(dct, NULL);
        CU_ASSERT_EQUAL(bytes_malloced, 0);
    }
}

void test_itor_survives_other_insert_and_remove()
{
    /* "k00" .. "k39". */
    char keys[40][4];
    for (unsigned i = 0; i < 40; ++i)
        snprintf(keys[i], sizeof(keys[i]), "k%02u", i);

    for (size_t c = 0; c < NUM_CONTAINERS; ++c) {
        dict* dct = containers[c].new_dict();
        CU_ASSERT_PTR_NOT_NULL_FATAL(dct);
        const bool hashed = is_hashtable(dct) || is_hashtable2(dct);
        for (unsigned i = 0; i < 30; ++i)
            CU_ASSERT_TRUE(dict_insert(dct, keys[i]).inserted);
        dict_itor* itor = dict_itor_new(dct);
        CU_ASSERT_TRUE_FATAL(dict_itor_search(itor, keys[5]));

        /* Inserting another key leaves the iterator on its key, unless it
         * made a hash table grow. */
        const size_t size = hashed ? hash_dict_size(dct) : 0;
        CU_ASSERT_TRUE(dict_insert(dct, keys[35]).inserted);
        CU_ASSERT_TRUE(dict_verify(dct));
        if (hashed && hash_dict_size(dct) != size) {
            check_itor_invalid(itor);
            CU_ASSERT_TRUE_FATAL(dict_itor_search(itor, keys[5]));
        } else {
            if (!dict_itor_valid(itor))
                fprintf(stderr, "%s: insert invalidated the iterator\n",
                        containers[c].name);
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_PTR_EQUAL(dict_itor_key(itor), keys[5]);
        }

        /* Removing another key leaves the iterator on its key, except for
         * hashtable2, where removals move elements between slots. */
        CU_ASSERT_TRUE(dict_remove(dct, keys[20]).removed);
        CU_ASSERT_TRUE(dict_verify(dct));
        if (is_hashtable2(dct)) {
            check_itor_invalid(itor);
        } else {
            if (!dict_itor_valid(itor))
                fprintf(stderr, "%s: remove invalidated the iterator\n",
                        containers[c].name);
            CU_ASSERT_TRUE(dict_itor_valid(itor));
            CU_ASSERT_PTR_EQUAL(dict_itor_key(itor), keys[5]);
        }

        dict_itor_free(itor);
        dict_free(dct, NULL);
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
