/*
 * libdict -- red-black tree implementation.
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
 * cf. [Cormen, Leiserson, and Rivest 1990], [Guibas and Sedgewick, 1978]
 */

#include "rb_tree.h"

#include "dict_private.h"
#include "tree_common.h"

typedef struct rb_node rb_node;
struct rb_node {
    void*       key;
    void*       datum;
    intptr_t    color;
    rb_node*    llink;
    rb_node*    rlink;
};

#define RB_RED      0
#define RB_BLACK    1

#define PARENT(node) ((rb_node*)((node)->color & ~RB_BLACK))
#define COLOR(node) ((node)->color & RB_BLACK)

#define SET_RED(node) (node)->color &= (~(intptr_t)RB_BLACK)
#define SET_BLACK(node) (node)->color |= ((intptr_t)RB_BLACK)
#define SET_PARENT(node,p) (node)->color = COLOR(node) | (intptr_t)(p)

/* rb_node is handed to the generic routines in tree_common.c as though it were
 * a tree_node_base, so its first five members must sit at the same offsets.
 * The third of those, the parent pointer, additionally carries the colour in
 * its low bit, so ONLY this file's PARENT() macro may decode it: any
 * tree_common routine that reads |parent| directly (tree_traverse,
 * tree_select, tree_clear, tree_iterator_next/prev, tree_node_rot_*) would
 * dereference a tagged pointer and must be reimplemented here. */
_Static_assert(sizeof(intptr_t) == sizeof(rb_node*),
               "rb_node tags a parent pointer stored in an intptr_t");
_Static_assert(offsetof(rb_node, key)   == offsetof(tree_node_base, key),
               "rb_node must be layout-compatible with tree_node_base");
_Static_assert(offsetof(rb_node, datum) == offsetof(tree_node_base, datum),
               "rb_node must be layout-compatible with tree_node_base");
_Static_assert(offsetof(rb_node, color) == offsetof(tree_node_base, parent),
               "rb_node must be layout-compatible with tree_node_base");
_Static_assert(offsetof(rb_node, llink) == offsetof(tree_node_base, llink),
               "rb_node must be layout-compatible with tree_node_base");
_Static_assert(offsetof(rb_node, rlink) == offsetof(tree_node_base, rlink),
               "rb_node must be layout-compatible with tree_node_base");

struct rb_tree {
    TREE_FIELDS(rb_node);
};

struct rb_itor {
    TREE_ITERATOR_FIELDS(rb_tree, rb_node);
};

/* Vtable thunks. The vtable slot types take void*, and calling a function
 * through a function-pointer type that is not compatible with the function's
 * own type is undefined behaviour (C11 6.5.2.2p9), so each slot below gets a
 * thunk with the exact slot signature instead of a cast. A cast would also
 * hide a genuine signature mismatch. */
static dict_itor*         vt_rb_dict_itor_new(void* obj)                                         { return rb_dict_itor_new(obj); }
static size_t             vt_rb_tree_free(void* obj, dict_delete_func delete_func)               { return rb_tree_free(obj, delete_func); }
static dict_insert_result vt_rb_tree_insert(void* obj, void* key)                                { return rb_tree_insert(obj, key); }
static dict_remove_result vt_rb_tree_remove(void* obj, const void* key)                          { return rb_tree_remove(obj, key); }
static size_t             vt_rb_tree_clear(void* obj, dict_delete_func delete_func)              { return rb_tree_clear(obj, delete_func); }
static size_t             vt_rb_tree_traverse(void* obj, dict_visit_func visit, void* user_data) { return rb_tree_traverse(obj, visit, user_data); }
static bool               vt_rb_tree_select(void* obj, size_t n, const void** key, void** datum) { return rb_tree_select(obj, n, key, datum); }
static bool               vt_rb_tree_verify(const void* obj)                                     { return rb_tree_verify(obj); }
static void               vt_rb_itor_free(void* itor)                                            { rb_itor_free(itor); }
static bool               vt_rb_itor_valid(const void* itor)                                     { return rb_itor_valid(itor); }
static void               vt_rb_itor_invalidate(void* itor)                                      { rb_itor_invalidate(itor); }
static bool               vt_rb_itor_next(void* itor)                                            { return rb_itor_next(itor); }
static bool               vt_rb_itor_prev(void* itor)                                            { return rb_itor_prev(itor); }
static bool               vt_rb_itor_nextn(void* itor, size_t count)                             { return rb_itor_nextn(itor, count); }
static bool               vt_rb_itor_prevn(void* itor, size_t count)                             { return rb_itor_prevn(itor, count); }
static bool               vt_rb_itor_first(void* itor)                                           { return rb_itor_first(itor); }
static bool               vt_rb_itor_last(void* itor)                                            { return rb_itor_last(itor); }
static void*              vt_rb_itor_key(void* itor)                                             { return (void*)rb_itor_key(itor); }
static void**             vt_rb_itor_datum(void* itor)                                           { return rb_itor_datum(itor); }
static bool               vt_rb_itor_search(void* itor, const void* key)                         { return rb_itor_search(itor, key); }
static bool               vt_rb_itor_search_le(void* itor, const void* key)                      { return rb_itor_search_le(itor, key); }
static bool               vt_rb_itor_search_lt(void* itor, const void* key)                      { return rb_itor_search_lt(itor, key); }
static bool               vt_rb_itor_search_ge(void* itor, const void* key)                      { return rb_itor_search_ge(itor, key); }
static bool               vt_rb_itor_search_gt(void* itor, const void* key)                      { return rb_itor_search_gt(itor, key); }
static bool               vt_rb_itor_remove(void* itor)                                          { return rb_itor_remove(itor); }

static const dict_vtable rb_tree_vtable = {
    true,
    vt_rb_dict_itor_new,
    vt_rb_tree_free,
    vt_rb_tree_insert,
    tree_search,
    tree_search_le,
    tree_search_lt,
    tree_search_ge,
    tree_search_gt,
    vt_rb_tree_remove,
    vt_rb_tree_clear,
    vt_rb_tree_traverse,
    vt_rb_tree_select,
    tree_count,
    vt_rb_tree_verify,
};

static const itor_vtable rb_tree_itor_vtable = {
    vt_rb_itor_free,
    vt_rb_itor_valid,
    vt_rb_itor_invalidate,
    vt_rb_itor_next,
    vt_rb_itor_prev,
    vt_rb_itor_nextn,
    vt_rb_itor_prevn,
    vt_rb_itor_first,
    vt_rb_itor_last,
    vt_rb_itor_key,
    vt_rb_itor_datum,
    vt_rb_itor_search,
    vt_rb_itor_search_le,
    vt_rb_itor_search_lt,
    vt_rb_itor_search_ge,
    vt_rb_itor_search_gt,
    vt_rb_itor_remove,
    tree_iterator_compare_vt
};

static void rot_left(rb_tree* tree, rb_node* node);
static void rot_right(rb_tree* tree, rb_node* node);
static unsigned insert_fixup(rb_tree* tree, rb_node* node);
static unsigned delete_fixup(rb_tree* tree, rb_node* node, rb_node* parent, bool left);
static rb_node* node_new(void* key);
static rb_node* node_next(rb_node* node);
static rb_node* node_prev(rb_node* node);

rb_tree*
rb_tree_new(dict_compare_func cmp_func)
{
    ASSERT(cmp_func != NULL);

    rb_tree* tree = MALLOC(sizeof(*tree));
    if (tree) {
        tree->root = NULL;
        tree->count = 0;
        tree->cmp_func = cmp_func;
        tree->rotation_count = 0;
    }
    return tree;
}

dict*
rb_dict_new(dict_compare_func cmp_func)
{
    dict* dct = MALLOC(sizeof(*dct));
    if (dct) {
        if (!(dct->_object = rb_tree_new(cmp_func))) {
            FREE(dct);
            return NULL;
        }
        dct->_vtable = &rb_tree_vtable;
    }
    return dct;
}

size_t
rb_tree_free(rb_tree* tree, dict_delete_func delete_func) {
    const size_t count = rb_tree_clear(tree, delete_func);
    FREE(tree);
    return count;
}

size_t
rb_tree_clear(rb_tree* tree, dict_delete_func delete_func)
{
    const size_t count = tree->count;

    rb_node* node = tree->root;
    while (node) {
        if (node->llink || node->rlink) {
            node = node->llink ? node->llink : node->rlink;
            continue;
        }
        if (delete_func) delete_func(node->key, node->datum);
        rb_node* const parent = PARENT(node);
        /* Unlink before freeing: comparing a pointer to a freed object is UB. */
        *(parent ? (parent->llink == node ? &parent->llink : &parent->rlink) : &tree->root) = NULL;
        FREE(node);
        node = parent;
    }
    ASSERT(tree->root == NULL);
    tree->count = 0;
    tree->rotation_count = 0;
    return count;
}

void** rb_tree_search(rb_tree* tree, const void* key) { return tree_search(tree, key); }
void** rb_tree_search_le(rb_tree* tree, const void* key) { return tree_search_le(tree, key); }
void** rb_tree_search_lt(rb_tree* tree, const void* key) { return tree_search_lt(tree, key); }
void** rb_tree_search_ge(rb_tree* tree, const void* key) { return tree_search_ge(tree, key); }
void** rb_tree_search_gt(rb_tree* tree, const void* key) { return tree_search_gt(tree, key); }

dict_insert_result
rb_tree_insert(rb_tree* tree, void* key)
{
    int cmp = 0;    /* Quell GCC warning about uninitialized usage. */
    rb_node* node = tree->root;
    rb_node* parent = NULL;
    while (node) {
        cmp = tree->cmp_func(key, node->key);
        if (cmp < 0) {
            parent = node; node = node->llink;
        } else if (cmp > 0) {
            parent = node; node = node->rlink;
        } else
            return (dict_insert_result) { &node->datum, false };
    }

    if (!(node = node_new(key)))
        return (dict_insert_result) { NULL, false };

    SET_PARENT(node, parent);
    if (parent == NULL) {
        ASSERT(tree->root == NULL);
        tree->root = node;
        ASSERT(tree->count == 0);
        SET_BLACK(node);
    } else {
        if (cmp < 0)
            parent->llink = node;
        else
            parent->rlink = node;

        tree->rotation_count += insert_fixup(tree, node);
    }
    tree->count++;
    return (dict_insert_result) { &node->datum, true };
}

static unsigned
insert_fixup(rb_tree* tree, rb_node* node)
{
    unsigned rotations = 0;
    while (node != tree->root && COLOR(PARENT(node)) == RB_RED) {
        if (PARENT(node) == PARENT(PARENT(node))->llink) {
            rb_node* temp = PARENT(PARENT(node))->rlink;
            if (temp && COLOR(temp) == RB_RED) {
                SET_BLACK(temp);
                node = PARENT(node);
                SET_BLACK(node);
                node = PARENT(node);
                SET_RED(node);
            } else {
                if (node == PARENT(node)->rlink) {
                    node = PARENT(node);
                    rot_left(tree, node);
                    ++rotations;
                }
                temp = PARENT(node);
                SET_BLACK(temp);
                temp = PARENT(temp);
                SET_RED(temp);
                rot_right(tree, temp);
                ++rotations;
            }
        } else {
            rb_node* temp = PARENT(PARENT(node))->llink;
            if (temp && COLOR(temp) == RB_RED) {
                SET_BLACK(temp);
                node = PARENT(node);
                SET_BLACK(node);
                node = PARENT(node);
                SET_RED(node);
            } else {
                if (node == PARENT(node)->llink) {
                    node = PARENT(node);
                    rot_right(tree, node);
                    ++rotations;
                }
                temp = PARENT(node);
                SET_BLACK(temp);
                temp = PARENT(temp);
                SET_RED(temp);
                rot_left(tree, temp);
                ++rotations;
            }
        }
    }

    SET_BLACK(tree->root);
    return rotations;
}

static void
remove_node(rb_tree* tree, rb_node* node)
{
    rb_node* out;
    if (!node->llink || !node->rlink) {
        out = node;
    } else {
        out = tree_node_min(node->rlink);
        void *tmp;
        SWAP(node->key, out->key, tmp);
        SWAP(node->datum, out->datum, tmp);
    }

    rb_node* const x = out->llink ? out->llink : out->rlink;
    rb_node* const xp = PARENT(out);
    if (x)
        SET_PARENT(x, xp);
    bool left = xp && xp->llink == out;
    *(xp ? (xp->llink == out ? &xp->llink : &xp->rlink) : &tree->root) = x;

    if (COLOR(out) == RB_BLACK && tree->root)
        tree->rotation_count += delete_fixup(tree, x, xp, left);
    FREE(out);
    tree->count--;
}

dict_remove_result
rb_tree_remove(rb_tree* tree, const void* key)
{
    rb_node* node = tree_search_node(tree, key);
    if (!node)
        return (dict_remove_result) { NULL, NULL, false };
    dict_remove_result result = { node->key, node->datum, true };
    remove_node(tree, node);
    return result;
}

static unsigned
delete_fixup(rb_tree* tree, rb_node* node, rb_node* parent, bool left)
{
    unsigned rotations = 0;
    while (node != tree->root && (node == NULL || COLOR(node) == RB_BLACK)) {
        if (left) {
            rb_node* w = parent->rlink;
            if (COLOR(w) == RB_RED) {
                SET_BLACK(w);
                SET_RED(parent);
                rot_left(tree, parent); ++rotations;
                w = parent->rlink;
            }
            if ((w->llink == NULL || COLOR(w->llink) == RB_BLACK) &&
                (w->rlink == NULL || COLOR(w->rlink) == RB_BLACK)) {
                SET_RED(w);
                node = parent;
                parent = PARENT(parent);
                left = parent && parent->llink == node;
            } else {
                if (w->rlink == NULL || COLOR(w->rlink) == RB_BLACK) {
                    SET_BLACK(w->llink);
                    SET_RED(w);
                    rot_right(tree, w); ++rotations;
                    w = parent->rlink;
                }
                if (COLOR(parent) == RB_RED)
                    SET_RED(w);
                else
                    SET_BLACK(w);
                if (w->rlink)
                    SET_BLACK(w->rlink);
                SET_BLACK(parent);
                rot_left(tree, parent); ++rotations;
                break;
            }
        } else { // left == false
            rb_node* w = parent->llink;
            if (COLOR(w) == RB_RED) {
                SET_BLACK(w);
                SET_RED(parent);
                rot_right(tree, parent); ++rotations;
                w = parent->llink;
            }
            if ((w->llink == NULL || COLOR(w->llink) == RB_BLACK) &&
                (w->rlink == NULL || COLOR(w->rlink) == RB_BLACK)) {
                SET_RED(w);
                node = parent;
                parent = PARENT(parent);
                left = parent && parent->llink == node;
            } else {
                if (w->llink == NULL || COLOR(w->llink) == RB_BLACK) {
                    SET_BLACK(w->rlink);
                    SET_RED(w);
                    rot_left(tree, w); ++rotations;
                    w = parent->llink;
                }
                if (COLOR(parent) == RB_RED)
                    SET_RED(w);
                else
                    SET_BLACK(w);
                if (w->llink)
                    SET_BLACK(w->llink);
                SET_BLACK(parent);
                rot_right(tree, parent); ++rotations;
                break;
            }
        }
    }

    if (node) {
        SET_BLACK(node);
    }
    return rotations;
}

size_t rb_tree_count(const rb_tree* tree) { return tree_count(tree); }
size_t rb_tree_min_path_length(const rb_tree* tree) { return tree_min_path_length(tree); }
size_t rb_tree_max_path_length(const rb_tree* tree) { return tree_max_path_length(tree); }
size_t rb_tree_total_path_length(const rb_tree* tree) { return tree_total_path_length(tree); }

size_t
rb_tree_traverse(rb_tree* tree, dict_visit_func visit, void* user_data)
{
    if (tree->root == NULL)
        return 0;

    size_t count = 0;
    rb_node* node = tree_node_min(tree->root);
    for (; node != NULL; node = node_next(node)) {
        ++count;
        if (!visit(node->key, node->datum, user_data)) break;
    }
    return count;
}

bool
rb_tree_select(rb_tree *tree, size_t n, const void **key, void **datum)
{
    if (n >= tree->count) {
        if (key)
            *key = NULL;
        if (datum)
            *datum = NULL;
        return false;
    }
    rb_node *node;
    if (n >= tree->count / 2) {
        node = tree_node_max(tree->root);
        n = tree->count - 1 - n;
        while (n--)
            node = node_prev(node);
    } else {
        node = tree_node_min(tree->root);
        while (n--)
            node = node_next(node);
    }
    if (key)
        *key = node->key;
    if (datum)
        *datum = node->datum;
    return true;
}

static void
rot_left(rb_tree* tree, rb_node* node)
{
    rb_node* const rlink = node->rlink;
    if ((node->rlink = rlink->llink) != NULL)
        SET_PARENT(rlink->llink, node);
    rb_node* const parent = PARENT(node);
    SET_PARENT(rlink, parent);
    *(parent ? (parent->llink == node ? &parent->llink : &parent->rlink) : &tree->root) = rlink;
    rlink->llink = node;
    SET_PARENT(node, rlink);
}

static void
rot_right(rb_tree* tree, rb_node* node)
{
    rb_node* const llink = node->llink;
    if ((node->llink = llink->rlink) != NULL)
        SET_PARENT(llink->rlink, node);
    rb_node* const parent = PARENT(node);
    SET_PARENT(llink, parent);
    *(parent ? (parent->llink == node ? &parent->llink : &parent->rlink) : &tree->root) = llink;
    llink->rlink = node;
    SET_PARENT(node, llink);
}

static rb_node*
node_new(void* key)
{
    rb_node* node = MALLOC(sizeof(*node));
    if (node) {
        /* The low bit of the parent pointer holds the colour, so a node must
         * be at least 2-byte aligned. dict_malloc_func is user-replaceable,
         * and an ASSERT would vanish under NDEBUG and let a misaligned
         * allocator corrupt every parent pointer silently. */
        if ((((uintptr_t) node) & 1) != 0) {
            fprintf(stderr, "%s:%d (%s) dict_malloc_func returned an odd "
                    "pointer: %p\n", __FILE__, __LINE__, __func__, (void*) node);
            abort();
        }
        node->key = key;
        node->datum = NULL;
        node->color = RB_RED; /* Also initializes parent to NULL */
        node->llink = NULL;
        node->rlink = NULL;
    }
    return node;
}

static rb_node*
node_next(rb_node* node)
{
    if (node->rlink) {
        for (node = node->rlink; node->llink; node = node->llink)
            /* void */;
    } else {
        rb_node* temp = PARENT(node);
        while (temp && temp->rlink == node) {
            node = temp;
            temp = PARENT(temp);
        }
        node = temp;
    }

    return node;
}

static rb_node*
node_prev(rb_node* node)
{
    if (node->llink) {
        for (node = node->llink; node->rlink; node = node->rlink)
            /* void */;
    } else {
        rb_node* temp = PARENT(node);
        while (temp && temp->llink == node) {
            node = temp;
            temp = PARENT(temp);
        }
        node = temp;
    }

    return node;
}

static bool
node_verify(const rb_tree* tree, const rb_node* parent, const rb_node* node,
        unsigned black_node_count, unsigned leaf_black_node_count,
        const void* lower, const void* upper, size_t* count)
{
    if (parent == NULL) {
        VERIFY(tree->root == node);
        VERIFY(node == NULL || COLOR(node) == RB_BLACK);
    } else {
        VERIFY(parent->llink == node || parent->rlink == node);
    }
    if (!node) {
        /* NIL is the leaf for black-height purposes, so the comparison
         * belongs here. Checking only at nodes with no children left every
         * path that terminates at the absent child of a *one-child* node
         * unverified -- which is most of them. */
        VERIFY(black_node_count == leaf_black_node_count);
        return true;
    }
    VERIFY(PARENT(node) == parent);
    /* Global BST ordering. Comparing a node only against its immediate parent
     * would accept an entire subtree placed on the wrong side. */
    if (lower)
        VERIFY(tree->cmp_func(lower, node->key) < 0);
    if (upper)
        VERIFY(tree->cmp_func(upper, node->key) > 0);
    if (COLOR(node) == RB_RED) {
        /* Verify that every child of a red node is black. */
        if (node->llink)
            VERIFY(COLOR(node->llink) == RB_BLACK);
        if (node->rlink)
            VERIFY(COLOR(node->rlink) == RB_BLACK);
    } else {
        black_node_count++;
    }
    *count += 1;
    bool l = node_verify(tree, node, node->llink, black_node_count,
                         leaf_black_node_count, lower, node->key, count);
    bool r = node_verify(tree, node, node->rlink, black_node_count,
                         leaf_black_node_count, node->key, upper, count);
    return l && r;
}

bool
rb_tree_verify(const rb_tree* tree)
{
    if (tree->root)
        VERIFY(COLOR(tree->root) == RB_BLACK);
    unsigned leaf_black_node_count = 0;
    if (tree->root) {
        VERIFY(tree->count > 0);
        for (rb_node* node = tree->root; node; node = node->llink)
            if (COLOR(node) == RB_BLACK)
                leaf_black_node_count++;
    } else {
        VERIFY(tree->count == 0);
    }
    size_t count = 0;
    if (!node_verify(tree, NULL, tree->root, 0, leaf_black_node_count,
                     NULL, NULL, &count))
        return false;
    VERIFY(tree->count == count);
    return true;
}

rb_itor*
rb_itor_new(rb_tree* tree)
{
    rb_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        itor->tree = tree;
        itor->node = NULL;
    }
    return itor;
}

dict_itor*
rb_dict_itor_new(rb_tree* tree)
{
    dict_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        if (!(itor->_itor = rb_itor_new(tree))) {
            FREE(itor);
            return NULL;
        }
        itor->_vtable = &rb_tree_itor_vtable;
    }
    return itor;
}

void rb_itor_free(rb_itor* itor) { tree_iterator_free(itor); }
bool rb_itor_valid(const rb_itor* itor) { return tree_iterator_valid(itor); }
void rb_itor_invalidate(rb_itor* itor) { tree_iterator_invalidate(itor); }

bool
rb_itor_next(rb_itor* itor)
{
    if (itor->node)
        itor->node = node_next(itor->node);
    return itor->node != NULL;
}

bool
rb_itor_prev(rb_itor* itor)
{
    if (itor->node)
        itor->node = node_prev(itor->node);
    return itor->node != NULL;
}

bool
rb_itor_nextn(rb_itor* itor, size_t count)
{
    while (count--) {
        if (!rb_itor_next(itor))
            return false;
    }
    return itor->node != NULL;
}

bool
rb_itor_prevn(rb_itor* itor, size_t count)
{
    while (count--) {
        if (!rb_itor_prev(itor))
            return false;
    }
    return itor->node != NULL;
}

bool rb_itor_first(rb_itor* itor) { return tree_iterator_first(itor); }
bool rb_itor_last(rb_itor* itor) { return tree_iterator_last(itor); }
bool rb_itor_search(rb_itor* itor, const void* key) { return tree_iterator_search(itor, key); }
bool rb_itor_search_le(rb_itor* itor, const void* key) { return tree_iterator_search_le(itor, key); }
bool rb_itor_search_lt(rb_itor* itor, const void* key) { return tree_iterator_search_lt(itor, key); }
bool rb_itor_search_ge(rb_itor* itor, const void* key) { return tree_iterator_search_ge(itor, key); }
bool rb_itor_search_gt(rb_itor* itor, const void* key) { return tree_iterator_search_gt(itor, key); }
const void* rb_itor_key(const rb_itor* itor) { return tree_iterator_key(itor); }
void** rb_itor_datum(rb_itor* itor) { return tree_iterator_datum(itor); }
int rb_itor_compare(const rb_itor* i1, const rb_itor* i2) { return tree_iterator_compare(i1, i2); }

bool
rb_itor_remove(rb_itor* it)
{
    if (!it->node)
        return false;
    remove_node(it->tree, it->node);
    it->node = NULL;
    return true;
}
