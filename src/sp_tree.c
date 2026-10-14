/*
 * libdict -- splay tree implementation.
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
 * cf. [Sleator and Tarjan, 1985], [Tarjan 1985], [Tarjan 1983]
 *
 * A single operation on a splay tree has a worst-case time complexity of O(N),
 * but a series of M operations have a time complexity of O(M lg N), and thus
 * the amortized time complexity of an operation is O(lg N). More specifically,
 * a series of M operations on a tree with N nodes will run in O((N+M)lg(N+M))
 * time. Splay trees work by "splaying" a node up the tree using a series of
 * rotations until it is the root each time it is accessed. They are much
 * simpler to code than most balanced trees, because there is no strict
 * requirement about maintaining a balance scheme among nodes. When inserting
 * and searching, we simply splay the node in question up until it becomes the
 * root of the tree.
 *
 * This implementation is a bottom-up, move-to-root splay tree.
 */

/* TODO: rather than splay after the fact, use the splay operation to traverse
 * the tree during insert, search, delete, etc. */

#include "sp_tree.h"

#include "dict_private.h"
#include "tree_common.h"

typedef struct sp_node sp_node;
struct sp_node {
    TREE_NODE_FIELDS(sp_node);
};

struct sp_tree {
    TREE_FIELDS(sp_node);
};

struct sp_itor {
    TREE_ITERATOR_FIELDS(sp_tree, sp_node);
};

/* Vtable thunks. The vtable slot types take void*, and calling a function
 * through a function-pointer type that is not compatible with the function's
 * own type is undefined behaviour (C11 6.5.2.2p9), so each slot below gets a
 * thunk with the exact slot signature instead of a cast. A cast would also
 * hide a genuine signature mismatch. */
static dict_itor*         vt_sp_dict_itor_new(void* obj)                   { return sp_dict_itor_new(obj); }
static dict_insert_result vt_sp_tree_insert(void* obj, void* key)          { return sp_tree_insert(obj, key); }
static void**             vt_sp_tree_search(void* obj, const void* key)    { return sp_tree_search(obj, key); }
static void**             vt_sp_tree_search_le(void* obj, const void* key) { return sp_tree_search_le(obj, key); }
static void**             vt_sp_tree_search_lt(void* obj, const void* key) { return sp_tree_search_lt(obj, key); }
static void**             vt_sp_tree_search_ge(void* obj, const void* key) { return sp_tree_search_ge(obj, key); }
static void**             vt_sp_tree_search_gt(void* obj, const void* key) { return sp_tree_search_gt(obj, key); }
static dict_remove_result vt_sp_tree_remove(void* obj, const void* key)    { return sp_tree_remove(obj, key); }
static bool               vt_sp_tree_verify(const void* obj)               { return sp_tree_verify(obj); }
static bool               vt_sp_itor_search(void* itor, const void* key)   { return sp_itor_search(itor, key); }
static bool               vt_sp_itor_remove(void* itor)                    { return sp_itor_remove(itor); }

static const dict_vtable sp_tree_vtable = {
    true,
    vt_sp_dict_itor_new,
    tree_free,
    vt_sp_tree_insert,
    vt_sp_tree_search,
    vt_sp_tree_search_le,
    vt_sp_tree_search_lt,
    vt_sp_tree_search_ge,
    vt_sp_tree_search_gt,
    vt_sp_tree_remove,
    tree_clear,
    tree_traverse,
    tree_select,
    tree_count,
    vt_sp_tree_verify,
};

static const itor_vtable sp_tree_itor_vtable = {
    tree_iterator_free,
    tree_iterator_valid,
    tree_iterator_invalidate,
    tree_iterator_next,
    tree_iterator_prev,
    tree_iterator_nextn,
    tree_iterator_prevn,
    tree_iterator_first,
    tree_iterator_last,
    tree_iterator_key_vt,
    tree_iterator_datum,
    vt_sp_itor_search,
    tree_iterator_search_le,
    tree_iterator_search_lt,
    tree_iterator_search_ge,
    tree_iterator_search_gt,
    vt_sp_itor_remove,
    tree_iterator_compare_vt
};

static sp_node* node_new(void* key);
static void splay(sp_tree* t, sp_node* n);

sp_tree*
sp_tree_new(dict_compare_func cmp_func)
{
    ASSERT(cmp_func != NULL);

    sp_tree* tree = MALLOC(sizeof(*tree));
    if (tree) {
        tree->root = NULL;
        tree->count = 0;
        tree->cmp_func = cmp_func;
        tree->rotation_count = 0;
    }
    return tree;
}

dict*
sp_dict_new(dict_compare_func cmp_func)
{
    dict* dct = MALLOC(sizeof(*dct));
    if (dct) {
        if (!(dct->_object = sp_tree_new(cmp_func))) {
            FREE(dct);
            return NULL;
        }
        dct->_vtable = &sp_tree_vtable;
    }
    return dct;
}

size_t sp_tree_free(sp_tree* tree, dict_delete_func delete_func) { return tree_free(tree, delete_func); }
size_t sp_tree_clear(sp_tree* tree, dict_delete_func delete_func) { return tree_clear(tree, delete_func); }

static void
splay(sp_tree* t, sp_node* n)
{
    unsigned rotations = 0;
    for (;;) {
        sp_node* p = n->parent;
        if (!p)
            break;

        sp_node* pp = p->parent;
        if (!pp) {
            /* Parent is the root; simply rotate root left or right so that |n|
            * becomes new root. */
            if (p->llink == n) {
                if ((p->llink = n->rlink) != NULL)
                    p->llink->parent = p;
                n->rlink = p;
            } else {
                if ((p->rlink = n->llink) != NULL)
                    p->rlink->parent = p;
                n->llink = p;
            }
            p->parent = n;
            t->root = n;
            n->parent = NULL;
            rotations += 1;
            break;
        }

        rotations += 2;
        sp_node* ppp = pp->parent;
        if (p->llink == n) {
            if (pp->llink == p) {
                /* Rotate parent right, then node right. */
                sp_node* const pr = p->rlink;
                p->rlink = pp;
                pp->parent = p;
                if ((pp->llink = pr) != NULL)
                    pr->parent = pp;

                sp_node* const nr = n->rlink;
                n->rlink = p;
                p->parent = n;
                if ((p->llink = nr) != NULL)
                    nr->parent = p;
            } else {
                /* Rotate node right, then parent left. */
                sp_node* const nr = n->rlink;
                n->rlink = p;
                p->parent = n;
                if ((p->llink = nr) != NULL)
                    nr->parent = p;

                sp_node* const nl = n->llink;
                n->llink = pp;
                pp->parent = n;
                if ((pp->rlink = nl) != NULL)
                    nl->parent = pp;
            }
        } else {
            if (pp->rlink == p) {
                /* Rotate parent left, then node left. */
                sp_node* const pl = p->llink;
                p->llink = pp;
                pp->parent = p;
                if ((pp->rlink = pl) != NULL)
                    pl->parent = pp;

                sp_node* const nl = n->llink;
                n->llink = p;
                p->parent = n;
                if ((p->rlink = nl) != NULL)
                    nl->parent = p;
            } else {
                /* Rotate node left, then parent right. */
                sp_node* const nl = n->llink;
                n->llink = p;
                p->parent = n;
                if ((p->rlink = nl) != NULL)
                    nl->parent = p;

                sp_node* const nr = n->rlink;
                n->rlink = pp;
                pp->parent = n;
                if ((pp->llink = nr) != NULL)
                    nr->parent = pp;
            }
        }
        n->parent = ppp;
        if (ppp) {
            if (ppp->llink == pp)
                ppp->llink = n;
            else
                ppp->rlink = n;
        } else {
            t->root = n;
            break;
        }
    }
    t->rotation_count += rotations;
}

dict_insert_result
sp_tree_insert(sp_tree* tree, void* key)
{
    int cmp = 0;
    sp_node* node = tree->root;
    sp_node* parent = NULL;
    while (node) {
        cmp = tree->cmp_func(key, node->key);
        if (cmp < 0) {
            parent = node; node = node->llink;
        } else if (cmp > 0) {
            parent = node; node = node->rlink;
        } else {
            /* Splay on a duplicate too: every other accessor restructures, and
             * skipping it here breaks the amortized O(lg n) bound for repeated
             * inserts of an existing key. */
            splay(tree, node);
            return (dict_insert_result) { &node->datum, false };
        }
    }

    if (!(node = node_new(key)))
        return (dict_insert_result) { NULL, false };

    if (!(node->parent = parent)) {
        ASSERT(tree->count == 0);
        ASSERT(tree->root == NULL);
        tree->root = node;
        tree->count = 1;
    } else {
        if (cmp < 0)
            parent->llink = node;
        else
            parent->rlink = node;
        splay(tree, node);
        tree->count++;
    }
    ASSERT(tree->root == node);
    return (dict_insert_result) { &node->datum, true };
}

void**
sp_tree_search(sp_tree* tree, const void* key)
{
    sp_node* parent = NULL;
    for (sp_node* node = tree->root; node;) {
        parent = node;
        const int cmp = tree->cmp_func(key, node->key);
        if (cmp < 0)
            node = node->llink;
        else if (cmp)
            node = node->rlink;
        else {
            splay(tree, node);
            ASSERT(tree->root == node);
            return &node->datum;
        }
    }
    if (parent)
        splay(tree, parent);
    return NULL;
}

void**
sp_tree_search_le(sp_tree* tree, const void* key)
{
    sp_node* node = tree_search_le_node(tree, key);
    if (node) {
        splay(tree, node);
        ASSERT(tree->root == node);
        return &node->datum;
    }
    return NULL;
}

void**
sp_tree_search_lt(sp_tree* tree, const void* key)
{
    sp_node* node = tree_search_lt_node(tree, key);
    if (node) {
        splay(tree, node);
        ASSERT(tree->root == node);
        return &node->datum;
    }
    return NULL;
}

void**
sp_tree_search_ge(sp_tree* tree, const void* key)
{
    sp_node* node = tree_search_ge_node(tree, key);
    if (node) {
        splay(tree, node);
        ASSERT(tree->root == node);
        return &node->datum;
    }
    return NULL;
}

void**
sp_tree_search_gt(sp_tree* tree, const void* key)
{
    sp_node* node = tree_search_gt_node(tree, key);
    if (node) {
        splay(tree, node);
        ASSERT(tree->root == node);
        return &node->datum;
    }
    return NULL;
}

static void
remove_node(sp_tree* tree, sp_node* node)
{
    sp_node* out;
    if (!node->llink || !node->rlink) {
        out = node;
    } else {
        out = tree_node_min(node->rlink);
        void* tmp;
        SWAP(node->key, out->key, tmp);
        SWAP(node->datum, out->datum, tmp);
    }

    sp_node* const temp = out->llink ? out->llink : out->rlink;
    sp_node* const parent = out->parent;
    /* Splice before freeing: comparing a pointer to a freed object is UB. */
    *(parent ? (parent->llink == out ? &parent->llink : &parent->rlink) : &tree->root) = temp;
    FREE(out);
    if (temp)
        temp->parent = parent;
    if (parent)
        splay(tree, parent);
    tree->count--;
}

dict_remove_result
sp_tree_remove(sp_tree* tree, const void* key)
{
    sp_node* node = tree_search_node(tree, key);
    if (!node)
        return (dict_remove_result) { NULL, NULL, false };
    dict_remove_result result = { node->key, node->datum, true };
    remove_node(tree, node);
    return result;
}

size_t sp_tree_traverse(sp_tree* tree, dict_visit_func visit, void* user_data) { return tree_traverse(tree, visit, user_data); }
bool sp_tree_select(sp_tree* tree, size_t n, const void** key, void** datum) { return tree_select(tree, n, key, datum); }
size_t sp_tree_count(const sp_tree* tree) { return tree_count(tree); }
size_t sp_tree_min_path_length(const sp_tree* tree) { return tree_min_path_length(tree); }
size_t sp_tree_max_path_length(const sp_tree* tree) { return tree_max_path_length(tree); }
size_t sp_tree_total_path_length(const sp_tree* tree) { return tree_total_path_length(tree); }

static sp_node*
node_new(void* key)
{
    sp_node* node = MALLOC(sizeof(*node));
    if (node) {
        node->key = key;
        node->datum = NULL;
        node->parent = NULL;
        node->llink = NULL;
        node->rlink = NULL;
    }
    return node;
}

bool
sp_tree_verify(const sp_tree* tree)
{
    /* A splay tree has no shape invariant of its own beyond being a valid
     * search tree, so the shared iterative check is the whole job. It must be
     * iterative: ascending inserts make a chain as deep as the tree is large,
     * and recursion would overflow the stack on exactly that case. */
    return tree_verify_common(tree);
}

sp_itor*
sp_itor_new(sp_tree* tree)
{
    sp_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        itor->tree = tree;
        itor->node = NULL;
    }
    return itor;
}

dict_itor*
sp_dict_itor_new(sp_tree* tree)
{
    dict_itor* itor = MALLOC(sizeof(*itor));
    if (itor) {
        if (!(itor->_itor = sp_itor_new(tree))) {
            FREE(itor);
            return NULL;
        }
        itor->_vtable = &sp_tree_itor_vtable;
    }
    return itor;
}

void sp_itor_free(sp_itor* itor) { tree_iterator_free(itor); }
bool sp_itor_valid(const sp_itor* itor) { return tree_iterator_valid(itor); }
void sp_itor_invalidate(sp_itor* itor) { tree_iterator_invalidate(itor); }
bool sp_itor_next(sp_itor* itor) { return tree_iterator_next(itor); }
bool sp_itor_prev(sp_itor* itor) { return tree_iterator_prev(itor); }
bool sp_itor_nextn(sp_itor* itor, size_t count) { return tree_iterator_nextn(itor, count); }
bool sp_itor_prevn(sp_itor* itor, size_t count) { return tree_iterator_prevn(itor, count); }
bool sp_itor_first(sp_itor* itor) { return tree_iterator_first(itor); }
bool sp_itor_last(sp_itor* itor) { return tree_iterator_last(itor); }
/* TODO: use algorithm from sp_tree_search() */
bool sp_itor_search(sp_itor* itor, const void* key) { return tree_iterator_search(itor, key); }
bool sp_itor_search_le(sp_itor* itor, const void* key) { return tree_iterator_search_le(itor, key); }
bool sp_itor_search_lt(sp_itor* itor, const void* key) { return tree_iterator_search_lt(itor, key); }
bool sp_itor_search_ge(sp_itor* itor, const void* key) { return tree_iterator_search_ge(itor, key); }
bool sp_itor_search_gt(sp_itor* itor, const void* key) { return tree_iterator_search_gt(itor, key); }
const void* sp_itor_key(const sp_itor* itor) { return tree_iterator_key(itor); }
void** sp_itor_datum(sp_itor* itor) { return tree_iterator_datum(itor); }

int
sp_itor_compare(const sp_itor* i1, const sp_itor* i2)
{
    return tree_iterator_compare(i1, i2);
}

bool
sp_itor_remove(sp_itor* itor)
{
    if (!itor->node)
        return false;
    remove_node(itor->tree, itor->node);
    itor->node = NULL;
    return true;
}
