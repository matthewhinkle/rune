/**
 * Generic collections: dynamic array and lock-free queue.
 *
 * Provides:
 *   - Dynamically-sized list with grow/shrink semantics
 *   - Lock-free bounded queue for concurrent access
 *   - Generic type support via macro-based template expansion
 *   - Sentinel type checking via #define guard macros
 *
 * Quick Reference:
 *
 *   List API
 *   -------------------------------------------------------------------------------------------------------------------
 *   list(type, ...)          Create list (with optional initial values)
 *   list_free(lst)           Free list memory
 *   list_get(lst, idx)       Get element at index
 *   list_add(lst, item)      Append item to end
 *   list_insert(
 *      lst,
 *      idx,
 *      item
 *   )                        Insert item at index
 *   list_remove(lst, idx)    Remove item at index
 *   list_grow(lst)           Ensure capacity for next item
 *   list_shrink(lst)         Reduce capacity if sparse
 *   list_resize(lst, cap)    Set exact capacity
 *
 *   Lock-Free Queue API
 *   -------------------------------------------------------------------------------------------------------------------
 *   lfq(type, cap, ...)      Create queue with initial values
 *   lfq_free(q)              Free queue memory
 *   lfq_capacity(q)          Get max capacity
 *   lfq_depth(q)             Get number of items in queue
 *   lfq_empty(q)             Check if queue is empty
 *   lfq_full(q)              Check if queue is full
 *   lfq_peek(q)              Get front item without removing
 *   lfq_push(q, item)        Add item to back
 *   lfq_pop(q)               Remove and return front item
 *   lfq_clear(q)             Remove all items
 *
 * Example:
 *   // List usage
 *   typedef struct { int x; } Point;
 *   LIST(Point) lst = list(Point);
 *   list_add(&lst, (Point){1, 2});
 *   list_add(&lst, (Point){3, 4});
 *   Point p = list_get(&lst, 0);  // {1, 2}
 *   list_free(&lst);
 *
 *   // Lock-free queue usage
 *   LFQ(int) q = lfq(int, 10);
 *   lfq_push(&q, 42);
 *   int val = lfq_pop(&q);
 *   lfq_free(&q);
 *
 * Note: List and LFQ require complete type definitions. For red-black trees, see tree.h.
 */

// ReSharper disable once CppMissingIncludeGuard
// ReSharper disable CppInconsistentNaming
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "r.h"

// Suppress pedantic warnings about GNU statement expressions (intentional, required for macro-based templates)
#pragma GCC diagnostic ignored "-Wpedantic"

// =====================================================================================================================
// List
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

#ifndef RUNE_LIST_API
#define RUNE_LIST_API

#define LIST(type) R_GLUE(list_, type)
#define R_LIST_OF(type) R_GLUE(LIST(type), _of)

#define list(type, ...) R_LIST_OF(type)((type[]){__VA_ARGS__}, sizeof((type[]){__VA_ARGS__}) / sizeof(type))

#define list_free(lst)                                                                                                 \
    ({                                                                                                                 \
        if ((lst)->data != nullptr) {                                                                                  \
            mem_free((lst)->data, (lst)->capacity * list_type_size(lst));                                              \
            (lst)->data = nullptr;                                                                                     \
        }                                                                                                              \
        (lst)->size = 0;                                                                                               \
        (lst)->capacity = 0;                                                                                           \
    })

#define list_type_size(lst) sizeof(typeof_unqual(*(lst)->data))

#define list_get(lst, idx) ((lst)->data[(idx)])

#define list_resize(lst, new_capacity)                                                                                 \
    ({                                                                                                                 \
        void * data =                                                                                                  \
            mem_realloc((lst)->data, (lst)->capacity * list_type_size(lst), (new_capacity) * list_type_size(lst));     \
        (lst)->capacity = new_capacity;                                                                                \
        (lst)->data = data;                                                                                            \
    })

#define list_grow(lst)                                                                                                 \
    ({                                                                                                                 \
        if ((lst)->capacity == 0) {                                                                                    \
            (lst)->capacity = 4;                                                                                       \
            (lst)->data = mem_alloc_zero((lst)->capacity * list_type_size(lst));                                       \
        }                                                                                                              \
        while ((lst)->size >= (lst)->capacity) {                                                                       \
            list_resize((lst), (lst)->capacity << 1);                                                                  \
        }                                                                                                              \
    })

#define list_shrink(lst)                                                                                               \
    ({                                                                                                                 \
        while ((lst)->size < (lst)->capacity >> 2) {                                                                   \
            list_resize((lst), (lst)->capacity >> 1);                                                                  \
        }                                                                                                              \
    })

#define list_add(lst, item)                                                                                            \
    ({                                                                                                                 \
        const size_t idx = (lst)->size;                                                                                \
        (lst)->size++;                                                                                                 \
        list_grow((lst));                                                                                              \
        (lst)->data[idx] = (item);                                                                                     \
    })

#define list_insert(lst, idx, item)                                                                                    \
    ({                                                                                                                 \
        (lst)->size++;                                                                                                 \
        list_grow((lst));                                                                                              \
        const size_t tail = (lst)->size - (idx);                                                                       \
        if (tail > 0) {                                                                                                \
            void * dst = &((lst)->data)[(idx) + 1];                                                                    \
            void * src = &((lst)->data)[(idx)];                                                                        \
            memmove(dst, src, tail * list_type_size(lst));                                                             \
        }                                                                                                              \
        (lst)->data[(idx)] = (item);                                                                                   \
    })

#define list_remove(lst, idx)                                                                                          \
    ({                                                                                                                 \
        auto R_UNIQUE(removed) = (lst)->data[(idx)];                                                                   \
        const size_t tail = (lst)->size - (idx) - 1;                                                                   \
        if (tail > 0) {                                                                                                \
            void * dst = &((lst)->data)[(idx)];                                                                        \
            void * src = &((lst)->data)[(idx) + 1];                                                                    \
            memmove(dst, src, tail * list_type_size(lst));                                                             \
        }                                                                                                              \
        (lst)->size--;                                                                                                 \
        list_shrink((lst));                                                                                            \
        R_UNIQUE(removed);                                                                                             \
    })

#endif // RUNE_LIST_API

// Type definition and implementation
// ---------------------------------------------------------------------------------------------------------------------

#ifdef T

typedef struct {
    T * data;
    size_t size;
    size_t capacity;
} LIST(T);

static LIST(T) R_LIST_OF(T)(const T * items, size_t count) {
    LIST(T) lst = {.data = nullptr, .size = 0, .capacity = 0};
    for (size_t i = 0; i < count; i++) {
        list_add(&lst, items[i]);
    }
    return lst;
}

#endif // T

// =====================================================================================================================
// Lock-Free Queue
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

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

#define lfq_depth(q) ((atomic_load(&(q)->tail) - atomic_load(&(q)->head)) % (q)->capacity)

#define lfq_empty(q) (atomic_load(&(q)->head) == atomic_load(&(q)->tail))

#define lfq_full(q) (((atomic_load(&(q)->tail) + 1) % (q)->capacity) == atomic_load(&(q)->head))

#define lfq_peek(q)                                                                                                    \
    ({                                                                                                                 \
        const size_t head = atomic_load(&(q)->head);                                                                   \
        typeof((q)->data[0]) result = (typeof((q)->data[0])){0};                                                       \
        if (head != atomic_load(&(q)->tail)) {                                                                         \
            result = (q)->data[head];                                                                                  \
        }                                                                                                              \
        result;                                                                                                        \
    })

#define lfq_clear(q)                                                                                                   \
    ({                                                                                                                 \
        atomic_store(&(q)->head, 0);                                                                                   \
        atomic_store(&(q)->tail, 0);                                                                                   \
    })

#define lfq_push(q, item)                                                                                              \
    ({                                                                                                                 \
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

// Type definition and implementation
// ---------------------------------------------------------------------------------------------------------------------

#ifdef T

typedef struct {
    T * data;
    size_t capacity;
    R_Atomic(size_t) head;
    R_Atomic(size_t) tail;
} LFQ(T);

[[maybe_unused]]
static LFQ(T) R_LFQ_OF(T)(size_t capacity, const T * items, size_t count) {
    LFQ(T)
    q = {.data = mem_alloc_zero(capacity * sizeof(T)), .capacity = capacity, .head = 0, .tail = 0};
    for (size_t i = 0; i < count && i < capacity - 1; i++) {
        lfq_push(&q, items[i]);
    }
    return q;
}

#endif // T

// =====================================================================================================================
// Hash Map
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

#ifndef RUNE_MAP_API
#define RUNE_MAP_API

#define MAP(key_t, val_t) R_GLUE(map_, R_JOIN(key_t, val_t, _))

#endif // RUNE_MAP_API

#if defined(K) && defined(V)

typedef struct {

} MAP(K, V);

#endif // K and V
