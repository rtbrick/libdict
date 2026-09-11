/*
 * libdict - common definitions for binary search trees.
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

#include "tree_common.h"

#include <string.h>
#include "dict_private.h"

typedef struct tree_node {
    TREE_NODE_FIELDS(struct tree_node);
} tree_node;

typedef struct {
    TREE_FIELDS(tree_node);
} tree;

typedef struct {
    TREE_ITERATOR_FIELDS(tree, tree_node);
} tree_iterator;

void
tree_node_rot_left(void* Tree, void* Node)
{
    tree_node* const n = Node;
    tree_node* const nr = n->rlink;
    ASSERT(nr != NULL);
    if ((n->rlink = nr->llink) != NULL)
        n->rlink->parent = n;
    nr->llink = n;

    tree_node* p = n->parent;
    n->parent = nr;
    nr->parent = p;
    *(p == NULL ? &((tree*)Tree)->root : p->llink == n ? &p->llink : &p->rlink) = nr;
}

void
tree_node_rot_right(void* Tree, void* Node)
{
    tree_node* const n = Node;
    tree_node* const nl = n->llink;
    ASSERT(nl != NULL);
    if ((n->llink = nl->rlink) != NULL)
        n->llink->parent = n;
    nl->rlink = n;

    tree_node* const p = n->parent;
    n->parent = nl;
    nl->parent = p;
    *(p == NULL ? &((tree*)Tree)->root : p->llink == n ? &p->llink : &p->rlink) = nl;
}

void*
tree_node_prev(void* Node)
{
    tree_node* node = Node;
    if (node->llink)
        return tree_node_max(node->llink);
    tree_node* parent = node->parent;
    while (parent && parent->llink == node) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

void*
tree_node_next(void* Node)
{
    tree_node* node = Node;
    if (node->rlink)
        return tree_node_min(node->rlink);
    tree_node* parent = node->parent;
    while (parent && parent->rlink == node) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

void*
tree_node_min(void* Node)
{
    tree_node* node = Node;
    if (!node)
        return NULL;
    while (node->llink)
        node = node->llink;
    return node;
}

void*
tree_node_max(void* Node)
{
    tree_node* node = Node;
    if (!node)
        return NULL;
    while (node->rlink)
        node = node->rlink;
    return node;
}

void*
tree_search_node(void* Tree, const void* key)
{
    tree* t = Tree;
    for (tree_node* node = t->root; node;) {
        const int cmp = t->cmp_func(key, node->key);
        if (cmp < 0)
            node = node->llink;
        else if (cmp)
            node = node->rlink;
        else
            return node;
    }
    return NULL;
}

void**
tree_search(void* Tree, const void* key)
{
    tree_node* node = tree_search_node(Tree, key);
    return node ? &node->datum : NULL;
}

void*
tree_search_le_node(void* Tree, const void* key)
{
    tree* t = Tree;
    tree_node* node = t->root, *ret = NULL;
    while (node) {
        const int cmp = t->cmp_func(key, node->key);
        if (cmp == 0)
            return node;
        if (cmp < 0) {
            node = node->llink;
        } else {
            ret = node;
            node = node->rlink;
        }
    }
    return ret;
}

void**
tree_search_le(void* Tree, const void* key)
{
    tree_node* node = tree_search_le_node(Tree, key);
    return node ? &node->datum : NULL;
}

void*
tree_search_lt_node(void* Tree, const void* key)
{
    tree* t = Tree;
    tree_node* node = t->root, *ret = NULL;
    while (node) {
        const int cmp = t->cmp_func(key, node->key);
        if (cmp <= 0) {
            node = node->llink;
        } else {
            ret = node;
            node = node->rlink;
        }
    }
    return ret;
}

void**
tree_search_lt(void* Tree, const void* key)
{
    tree_node* node = tree_search_lt_node(Tree, key);
    return node ? &node->datum : NULL;
}

void*
tree_search_ge_node(void* Tree, const void* key)
{
    tree* t = Tree;
    tree_node* node = t->root, *ret = NULL;
    while (node) {
        const int cmp = t->cmp_func(key, node->key);
        if (cmp == 0) {
            return node;
        }
        if (cmp < 0) {
            ret = node;
            node = node->llink;
        } else {
            node = node->rlink;
        }
    }
    return ret;
}

void**
tree_search_ge(void* Tree, const void* key)
{
    tree_node* node = tree_search_ge_node(Tree, key);
    return node ? &node->datum : NULL;
}

void*
tree_search_gt_node(void* Tree, const void* key)
{
    tree* t = Tree;
    tree_node* node = t->root, *ret = NULL;
    while (node) {
        const int cmp = t->cmp_func(key, node->key);
        if (cmp < 0) {
            ret = node;
            node = node->llink;
        } else {
            node = node->rlink;
        }
    }
    return ret;
}

void**
tree_search_gt(void* Tree, const void* key)
{
    tree_node* node = tree_search_gt_node(Tree, key);
    return node ? &node->datum : NULL;
}

size_t
tree_traverse(void* Tree, dict_visit_func visit, void* user_data)
{
    ASSERT(visit != NULL);

    tree* t = Tree;
    size_t count = 0;
    if (t->root) {
        tree_node* node = tree_node_min(t->root);
        do {
            ++count;
            if (!visit(node->key, node->datum, user_data)) break;
            node = tree_node_next(node);
        } while (node);
    }
    return count;
}

bool
tree_select(void *Tree, size_t n, const void **key, void **datum)
{
    tree* t = Tree;
    if (n >= t->count) {
        if (key)
            *key = NULL;
        if (datum)
            *datum = NULL;
        return false;
    }
    tree_node* node;
    if (n >= t->count / 2) {
        node = tree_node_max(t->root);
        n = t->count - 1 - n;
        while (n--)
            node = tree_node_prev(node);
    } else {
        node = tree_node_min(t->root);
        while (n--)
            node = tree_node_next(node);
    }
    if (key)
        *key = node->key;
    if (datum)
        *datum = node->datum;
    return true;
}

size_t
tree_count(const void* Tree)
{
    return ((const tree*)Tree)->count;
}

/* Verifies the invariants every tree shares: root/parent link consistency,
 * global key ordering, and that |count| matches the number of nodes reachable.
 *
 * This is iterative for the same reason the path-length statistics are: a
 * degenerate sp_tree or tr_tree is as deep as it has nodes, so a recursive
 * checker would overflow the stack on the very trees most likely to be
 * malformed. It may only be used by trees with an untagged parent pointer --
 * hb_tree and rb_tree pack bits into that field and must verify themselves. */
bool
tree_verify_common(const void* Tree)
{
    const tree* t = Tree;

    VERIFY(t->root == NULL || t->root->parent == NULL);
    if (t->root) {
        VERIFY(t->count > 0);
    } else {
        VERIFY(t->count == 0);
    }

    /* (node, lower bound, upper bound) triples still to visit. A NULL bound
     * means unbounded on that side. */
    struct frame { const tree_node* node; const void* lower; const void* upper; };
    struct frame* stack = NULL;
    size_t size = 0, capacity = 0, count = 0;
    bool ok = true, oom = false;

#define PUSH(n, lo, hi) \
    do { \
        if (size == capacity) { \
            const size_t cap = capacity ? capacity * 2 : 32; \
            struct frame* items = MALLOC(cap * sizeof(*items)); \
            if (!items) { oom = true; break; } \
            if (stack) { \
                memcpy(items, stack, size * sizeof(*items)); \
                FREE(stack); \
            } \
            stack = items; \
            capacity = cap; \
        } \
        stack[size].node = (n); \
        stack[size].lower = (lo); \
        stack[size].upper = (hi); \
        ++size; \
    } while (0)

    if (t->root)
        PUSH(t->root, NULL, NULL);

    while (ok && !oom && size) {
        const struct frame top = stack[--size];
        const tree_node* const node = top.node;

        /* Global key ordering. Comparing a node only against its immediate
         * parent would accept an entire subtree placed on the wrong side. */
        if (top.lower && t->cmp_func(top.lower, node->key) >= 0) {
            ok = false;
            break;
        }
        if (top.upper && t->cmp_func(top.upper, node->key) <= 0) {
            ok = false;
            break;
        }
        if (node->llink && node->llink->parent != node) {
            ok = false;
            break;
        }
        if (node->rlink && node->rlink->parent != node) {
            ok = false;
            break;
        }
        ++count;
        if (node->llink)
            PUSH(node->llink, top.lower, node->key);
        if (!oom && node->rlink)
            PUSH(node->rlink, node->key, top.upper);
    }
#undef PUSH
    if (stack)
        FREE(stack);

    if (oom) {
        fprintf(stderr, "%s:%d (%s) out of memory\n", __FILE__, __LINE__, __func__);
        return false;
    }
    VERIFY(ok);
    VERIFY(t->count == count);
    return true;
}

size_t
tree_clear(void* Tree, dict_delete_func delete_func)
{
    tree* t = Tree;
    const size_t count = t->count;

    tree_node* node = t->root;
    while (node) {
        if (node->llink || node->rlink) {
            node = node->llink ? node->llink : node->rlink;
            continue;
        }
        if (delete_func) delete_func(node->key, node->datum);
        tree_node* const parent = node->parent;
        /* Unlink before freeing: comparing a pointer to a freed object is UB. */
        *(parent ? (parent->llink == node ? &parent->llink : &parent->rlink) : &t->root) = NULL;
        FREE(node);
        node = parent;
    }
    ASSERT(t->root == NULL);
    t->count = 0;
    return count;
}

size_t
tree_free(void* Tree, dict_delete_func delete_func)
{
    const size_t count = tree_clear(Tree, delete_func);
    FREE(Tree);
    return count;
}

/* The three path-length statistics below share one iterative traversal.
 *
 * They must not recurse: a degenerate sp_tree or tr_tree (e.g. a treap given a
 * constant priority function) is as deep as it has nodes, so recursion would
 * overflow the stack on a few hundred thousand elements. They must also not
 * walk parent pointers, because hb_tree and rb_tree pack bits into that field
 * and only their own PARENT() macro may decode it -- so the traversal keeps an
 * explicit, growable stack of (node, depth) pairs instead.
 *
 * Depths are 1-based, i.e. the root is at depth 1. */
typedef struct {
    const tree_node*    node;
    size_t              depth;
} node_depth;

typedef struct {
    node_depth* items;
    size_t      size;
    size_t      capacity;
} node_stack;

/* Returns false only if memory could not be obtained. */
static bool
node_stack_push(node_stack* stack, const tree_node* node, size_t depth)
{
    if (stack->size == stack->capacity) {
        const size_t capacity = stack->capacity ? stack->capacity * 2 : 32;
        node_depth* items = MALLOC(capacity * sizeof(*items));
        if (!items)
            return false;
        if (stack->items) {
            memcpy(items, stack->items, stack->size * sizeof(*items));
            FREE(stack->items);
        }
        stack->items = items;
        stack->capacity = capacity;
    }
    stack->items[stack->size].node = node;
    stack->items[stack->size].depth = depth;
    ++stack->size;
    return true;
}

/* Computes, over all nodes of |t|:
 *  |min_path|   the shallowest depth at which a child link is absent,
 *  |max_path|   the greatest node depth,
 *  |total_path| the sum of all node depths.
 * Any output pointer may be NULL. Returns false if the traversal ran out of
 * memory, in which case the outputs are not written. */
static bool
tree_path_lengths(const tree* t, size_t* min_path, size_t* max_path,
                  size_t* total_path)
{
    size_t min_len = 0, max_len = 0, total_len = 0;
    node_stack stack = { NULL, 0, 0 };
    bool ok = true;

    if (t->root && !node_stack_push(&stack, t->root, 1))
        ok = false;

    while (ok && stack.size) {
        const node_depth top = stack.items[--stack.size];
        const tree_node* const node = top.node;
        const size_t depth = top.depth;

        if (depth > max_len)
            max_len = depth;
        total_len += depth;
        if (!node->llink || !node->rlink) {
            if (!min_len || depth < min_len)
                min_len = depth;
        }
        if (node->llink && !node_stack_push(&stack, node->llink, depth + 1))
            ok = false;
        if (ok && node->rlink && !node_stack_push(&stack, node->rlink, depth + 1))
            ok = false;
    }
    if (stack.items)
        FREE(stack.items);

    if (!ok)
        return false;
    if (min_path)
        *min_path = min_len;
    if (max_path)
        *max_path = max_len;
    if (total_path)
        *total_path = total_len;
    return true;
}

size_t
tree_min_path_length(const void* Tree)
{
    size_t len = 0;
    tree_path_lengths(Tree, &len, NULL, NULL);
    return len;
}

size_t
tree_max_path_length(const void* Tree)
{
    size_t len = 0;
    tree_path_lengths(Tree, NULL, &len, NULL);
    return len;
}

size_t
tree_total_path_length(const void* Tree)
{
    size_t len = 0;
    tree_path_lengths(Tree, NULL, NULL, &len);
    return len;
}

bool
tree_iterator_valid(const void* Iterator)
{
    return ((const tree_iterator*)Iterator)->node != NULL;
}

void
tree_iterator_invalidate(void* Iterator)
{
    ((tree_iterator*)Iterator)->node = NULL;
}

void
tree_iterator_free(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    iterator->node = NULL;
    FREE(iterator);
}

bool
tree_iterator_next(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    if(iterator->node) {
        iterator->node = tree_node_next(iterator->node);
        if(iterator->node) return true;
    }
    return false;
}

bool
tree_iterator_prev(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    if(iterator->node) {
        iterator->node = tree_node_prev(iterator->node);
        if(iterator->node) return true;
    }
    return false;
}

bool
tree_iterator_nextn(void* Iterator, size_t count)
{
    tree_iterator* iterator = Iterator;
    while (iterator->node && count--)
        iterator->node = tree_node_next(iterator->node);
    return iterator->node != NULL;
}

bool
tree_iterator_prevn(void* Iterator, size_t count)
{
    tree_iterator* iterator = Iterator;
    while (iterator->node && count--)
        iterator->node = tree_node_prev(iterator->node);
    return iterator->node != NULL;
}

bool
tree_iterator_first(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_node_min(iterator->tree->root)) != NULL;
}

bool
tree_iterator_last(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_node_max(iterator->tree->root)) != NULL;
}

bool
tree_iterator_search(void* Iterator, const void* key)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_search_node(iterator->tree, key)) != NULL;
}

bool
tree_iterator_search_le(void* Iterator, const void* key)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_search_le_node(iterator->tree, key)) != NULL;
}

bool
tree_iterator_search_lt(void* Iterator, const void* key)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_search_lt_node(iterator->tree, key)) != NULL;
}

bool
tree_iterator_search_ge(void* Iterator, const void* key)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_search_ge_node(iterator->tree, key)) != NULL;
}

bool
tree_iterator_search_gt(void* Iterator, const void* key)
{
    tree_iterator* iterator = Iterator;
    return (iterator->node = tree_search_gt_node(iterator->tree, key)) != NULL;
}

int
tree_iterator_compare(const void* Iterator1, const void* Iterator2)
{
    const tree_iterator* itor1 = Iterator1;
    const tree_iterator* itor2 = Iterator2;
    ASSERT(itor1->tree == itor2->tree);
    if (!itor1->node)
        return !itor2->node ? 0 : -1;
    if (!itor2->node)
        return 1;
    return itor1->tree->cmp_func(itor1->node->key, itor2->node->key);
}

const void*
tree_iterator_key(const void* Iterator)
{
    const tree_iterator* iterator = Iterator;
    return iterator->node ? iterator->node->key : NULL;
}

void**
tree_iterator_datum(void* Iterator)
{
    tree_iterator* iterator = Iterator;
    return iterator->node ? &iterator->node->datum : NULL;
}
