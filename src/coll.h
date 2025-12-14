/*
 * Generic collection implementations with type-safe sentinels.
 */

// ReSharper disable once CppMissingIncludeGuard
// ReSharper disable CppInconsistentNaming
#include <assert.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "r.h"

// -------------------------------------------------------------------------------------------------
// list
// -------------------------------------------------------------------------------------------------

#ifndef RUNE_LIST_API
#define RUNE_LIST_API

#define LIST(type) R_GLUE(list_, type)
#define R_LIST_OF(type) R_GLUE(LIST(type), _of)

#define list(type, ...) R_LIST_OF(type)((type[]){__VA_ARGS__}, sizeof((type[]){__VA_ARGS__}) / sizeof(type))

#define list_empty(type) {.data = nullptr, .size = 0, .capacity = 0}

#define list_free(lst)                                                                                                 \
    ({                                                                                                                 \
        if ((lst) != nullptr) {                                                                                        \
            if ((lst)->data != nullptr) {                                                                              \
                mem_free((lst)->data, (lst)->capacity * list_type_size(lst));                                          \
                (lst)->data = nullptr;                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            (lst)->size = 0;                                                                                           \
            (lst)->capacity = 0;                                                                                       \
        }                                                                                                              \
    })

#define list_type_size(lst) sizeof(typeof_unqual(*(lst)->data))

#define list_get(lst, idx)                                                                                             \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
        assert((lst)->data != nullptr);                                                                                \
        assert((idx) < (lst)->size);                                                                                   \
        (lst)->data[(idx)];                                                                                            \
    })

#define list_resize(lst, new_capacity)                                                                                 \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
        assert((lst)->data != nullptr);                                                                                \
        assert((new_capacity) >= (lst)->size);                                                                         \
                                                                                                                       \
        void * data =                                                                                                  \
            mem_realloc((lst)->data, (lst)->capacity * list_type_size(lst), (new_capacity) * list_type_size(lst));     \
        (lst)->capacity = new_capacity;                                                                                \
        (lst)->data = data;                                                                                            \
    })

#define list_grow(lst)                                                                                                 \
    ({                                                                                                                 \
        if ((lst)->capacity <= 0) {                                                                                    \
            (lst)->capacity = 4;                                                                                       \
            (lst)->data = mem_alloc_zero((lst)->capacity * list_type_size(lst));                                       \
        }                                                                                                              \
                                                                                                                       \
        while ((lst)->size >= (lst)->capacity) {                                                                       \
            list_resize((lst), (lst)->capacity << 1);                                                                  \
        }                                                                                                              \
    })

#define list_shrink(lst)                                                                                               \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
                                                                                                                       \
        while ((lst)->size < (lst)->capacity >> 2) {                                                                   \
            list_resize((lst), (lst)->capacity >> 1);                                                                  \
        }                                                                                                              \
    })

#define list_add(lst, item)                                                                                            \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
                                                                                                                       \
        const size_t idx = (lst)->size;                                                                                \
        (lst)->size++;                                                                                                 \
        list_grow((lst));                                                                                              \
                                                                                                                       \
        (lst)->data[idx] = (item);                                                                                     \
    })

#define list_insert(lst, idx, item)                                                                                    \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
        assert((lst)->data != nullptr);                                                                                \
        assert((idx) <= (lst)->size);                                                                                  \
                                                                                                                       \
        (lst)->size++;                                                                                                 \
        list_grow((lst));                                                                                              \
                                                                                                                       \
        const size_t tail = (lst)->size - (idx);                                                                       \
        if (tail > 0) {                                                                                                \
            void * dst = &((lst)->data)[(idx) + 1];                                                                    \
            void * src = &((lst)->data)[(idx)];                                                                        \
            memmove(dst, src, tail * list_type_size(lst));                                                             \
        }                                                                                                              \
                                                                                                                       \
        (lst)->data[(idx)] = (item);                                                                                   \
    })

#define list_remove(lst, idx)                                                                                          \
    ({                                                                                                                 \
        assert((lst) != nullptr);                                                                                      \
        assert((lst)->data != nullptr);                                                                                \
        assert((idx) < (lst)->size);                                                                                   \
                                                                                                                       \
        auto R_UNIQUE(removed) = (lst)->data[(idx)];                                                                   \
                                                                                                                       \
        const size_t tail = (lst)->size - (idx) - 1;                                                                   \
        if (tail > 0) {                                                                                                \
            void * dst = &((lst)->data)[(idx)];                                                                        \
            void * src = &((lst)->data)[(idx) + 1];                                                                    \
            memmove(dst, src, tail * list_type_size(lst));                                                             \
        }                                                                                                              \
                                                                                                                       \
        (lst)->size--;                                                                                                 \
        list_shrink((lst));                                                                                            \
                                                                                                                       \
        R_UNIQUE(removed);                                                                                             \
    })

#endif // RUNE_LIST_API

#ifdef T

// List type definition and implementation
typedef struct {
    T * data;
    size_t size;
    size_t capacity;
} LIST(T);

static LIST(T) R_LIST_OF(T)(const T * items, size_t count) {
    LIST(T) lst = list_empty(T);
    for (size_t i = 0; i < count; i++) {
        list_add(&lst, items[i]);
    }
    return lst;
}

#endif // T

// -------------------------------------------------------------------------------------------------
// lock-free queue
// -------------------------------------------------------------------------------------------------

#ifndef RUNE_LFQ_API
#define RUNE_LFQ_API

#define LFQ(type) R_GLUE(lfq_, type)
#define R_LFQ_OF(type) R_GLUE(LFQ(type), _of)

#define lfq(type, cap, ...) R_LFQ_OF(type)((cap), (type[]){__VA_ARGS__}, sizeof((type[]){__VA_ARGS__}) / sizeof(type))

#define lfq_free(q)                                                                                                    \
    ({                                                                                                                 \
        if ((q) != nullptr) {                                                                                          \
            if ((q)->data != nullptr) {                                                                                \
                mem_free((q)->data, (q)->capacity * lfq_type_size(q));                                                 \
                (q)->data = nullptr;                                                                                   \
            }                                                                                                          \
                                                                                                                       \
            (q)->capacity = 0;                                                                                         \
            (q)->head = 0;                                                                                             \
            (q)->tail = 0;                                                                                             \
        }                                                                                                              \
    })

#define lfq_type_size(q) sizeof(typeof_unqual(*(q)->data))

#define lfq_capacity(q) (q)->capacity

#define lfq_depth(q)                                                                                                   \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        (atomic_load(&(q)->tail) - atomic_load(&(q)->head)) % (q)->capacity;                                           \
    })

#define lfq_empty(q)                                                                                                   \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        atomic_load(&(q)->head) == atomic_load(&(q)->tail);                                                            \
    })

#define lfq_full(q)                                                                                                    \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        (atomic_load(&(q)->tail) + 1) % (q)->capacity == atomic_load(&(q)->head);                                      \
    })

#define lfq_peek(q)                                                                                                    \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        const size_t head = atomic_load(&(q)->head);                                                                   \
        typeof((q)->data[0]) result = (typeof((q)->data[0])){0};                                                       \
        if (head != atomic_load(&(q)->tail)) {                                                                         \
            result = (q)->data[head];                                                                                  \
        }                                                                                                              \
        result;                                                                                                        \
    })

#define lfq_clear(q)                                                                                                   \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        atomic_store(&(q)->head, 0);                                                                                   \
        atomic_store(&(q)->tail, 0);                                                                                   \
    })

#define lfq_push(q, item)                                                                                              \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        const size_t tail = atomic_load(&(q)->tail);                                                                   \
        const size_t next_tail = (tail + 1) % (q)->capacity;                                                           \
        auto R_UNIQUE(result) = item;                                                                                  \
        if (next_tail == atomic_load(&(q)->head)) {                                                                    \
            err_set(R_ERR_QUEUE_FULL, nullptr);                                                                        \
            R_UNIQUE(result) = (typeof(item)){0};                                                                      \
        } else {                                                                                                       \
            (q)->data[tail] = (item);                                                                                  \
            atomic_store(&(q)->tail, next_tail);                                                                       \
        }                                                                                                              \
        R_UNIQUE(result);                                                                                              \
    })

#define lfq_pop(q)                                                                                                     \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        const size_t head = atomic_load(&(q)->head);                                                                   \
        typeof((q)->data[0]) item = (typeof((q)->data[0])){0};                                                         \
        if (head != atomic_load(&(q)->tail)) {                                                                         \
            item = (q)->data[head];                                                                                    \
            atomic_store(&(q)->head, (head + 1) % (q)->capacity);                                                      \
        } else {                                                                                                       \
            err_set(R_ERR_QUEUE_EMPTY, nullptr);                                                                       \
        }                                                                                                              \
        item;                                                                                                          \
    })

#define lfq_resize(q, new_capacity)                                                                                    \
    ({                                                                                                                 \
        assert((q) != nullptr);                                                                                        \
        assert((new_capacity) >= (q)->capacity);                                                                       \
        const size_t old_capacity = (q)->capacity;                                                                     \
        const size_t head = (q)->head;                                                                                 \
        const size_t tail = (q)->tail;                                                                                 \
        const size_t type_size = lfq_type_size(q);                                                                     \
        void * new_data = mem_realloc((q)->data, old_capacity * type_size, new_capacity * type_size);                  \
        if (tail < head) {                                                                                             \
            const size_t first = old_capacity - head;                                                                  \
            memmove((char *)new_data + first * type_size, new_data, tail * type_size);                                 \
        }                                                                                                              \
        (q)->data = new_data;                                                                                          \
        (q)->capacity = new_capacity;                                                                                  \
        (q)->head = 0;                                                                                                 \
        (q)->tail = (tail >= head) ? (tail - head) : (old_capacity - head + tail);                                     \
    })

#endif // RUNE_LFQ_API

#ifdef T

// Lock-free queue type definition and implementation
typedef struct {
    T * data;
    size_t capacity;
    R_Atomic(size_t) head;
    R_Atomic(size_t) tail;
} LFQ(T);

static LFQ(T) R_LFQ_OF(T)(size_t capacity, const T * items, size_t count) {
    LFQ(T)
    q = {.data = mem_alloc_zero(capacity * sizeof(T)), .capacity = capacity, .head = 0, .tail = 0};
    for (size_t i = 0; i < count && i < capacity - 1; i++) {
        lfq_push(&q, items[i]);
    }
    return q;
}

#endif // T

// -------------------------------------------------------------------------------------------------
// red black tree
// -------------------------------------------------------------------------------------------------

#ifndef RUNE_RBT_API
#define RUNE_RBT_API

enum rbt_color { R_(red), R_(black) };

enum rbt_dir { R_(left), R_(right) };

#define RBT(type) R_GLUE(rbt_, type)
#define RBT_NODE(type) R_GLUE(rbt_node_, type)
#define R_RBT_NEW(type) R_GLUE(RBT(type), _new)
#define R_RBT_NODE_NEW(type) R_GLUE(RBT_NODE(type), _new)

#define rbt(type, cmp_fn) R_RBT_NEW(type)(R_OPT(nullptr, __VA_ARGS__))
#define rbt_node(type, data) R_RBT_NODE_NEW(type)((data))

#define rbt_cmp(t, a, b) ((t)->cmp == nullptr ? ((a) > (b)) - ((a) < (b)) : (t)->cmp_fn((a), (b)))

#define rbt_parent(root, val, out_slot)                                                                                \
    ({                                                                                                                 \
        struct rb_node * parent = nullptr;                                                                             \
        struct rb_node * cur = (root);                                                                                 \
        while (cur != nullptr && rbt_cmp(t, val, cur->val) != 0) {                                                     \
            parent = cur;                                                                                              \
            if (rbt_cmp(t, val, cur->val) < 0) {                                                                       \
                cur = cur->left;                                                                                       \
            } else {                                                                                                   \
                cur = cur->right;                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        if ((out_slot) != nullptr) {                                                                                   \
            (*out_slot) = cur;                                                                                         \
        }                                                                                                              \
                                                                                                                       \
        /* return */ parent;                                                                                           \
    })

#define rbt_min(node)                                                                                                  \
    ({                                                                                                                 \
        struct rb_node * cur = (node);                                                                                 \
        while (cur != nullptr && cur->left != nullptr) {                                                               \
            cur = cur->left;                                                                                           \
        }                                                                                                              \
        /* return */ cur;                                                                                              \
    })

#define rbt_isleft(node) (node)->parent != nullptr && (node)->parent->left == (node)

#endif // RUNE_RBT_API

#ifdef T

struct RBT_NODE(T) {
    T data;
    enum rbt_color color;
    struct RBT_NODE(T) * parent;
    struct RBT_NODE(T) * left;
    struct RBT_NODE(T) * right;
};

typedef struct {
    struct RBT_NODE(T) * root;
    int (*cmp_fn)(T a, T b);
    struct RBT_NODE(T) * (*node_fn)(T data);
} RBT(T);

static struct RBT_NODE(T) * R_RBT_NODE_NEW(T)(T data) {
    struct RBT_NODE(T) * node = mem_alloc_zero(sizeof(struct RBT_NODE(T)));
    node->color = R_(red);
    node->data = data;
    return node;
}

static RBT(T) R_RBT_NEW(T)(int (*cmp_fn)(T a, T b)) {
    const RBT(T) rbt = {
        .root = nullptr,
        .cmp_fn = cmp_fn,
        .node_fn = R_RBT_NODE_NEW(T),
    };
    return rbt;
}

#endif // T
