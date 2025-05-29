/*
 * Common collection implementations.
 *
 * Example usage:
 * ```c
 * #define T int
 * #include "coll.h"
 *
 * typedef struct { int foo; int bar; } my_type;
 *
 * #define T my_type
 * #include "coll.h"
 *
 * LIST(int) my_ints = list(int, 1, 2, 3, 4, 5);
 * LIST(my_type) my_types = list(my_type, {1, 2}, {3, 4}, {5, 6});
 * ```
 *
 * Supported #defines:
 *      - `T` - the generic collection item type.
 */

// ReSharper disable once CppMissingIncludeGuard
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "std.h"

// -----
// array
// -----

#ifndef RUNE_ARRAY_API
#define RUNE_ARRAY_API

#endif // RUNE_ARRAY_API

// ----
// list
// ----

#ifndef RUNE_LIST_API
#define RUNE_LIST_API

#define LIST(type) GLUE(list_, type)
#define R_LIST_OF(type) GLUE(LIST(type), _of)

#define list(type, ...) R_LIST_OF(type)(__VA_ARGS__, R_END)

#define list_free(lst)                                                                             \
    FUNC({                                                                                         \
        if ((lst) != nullptr) {                                                                    \
            if ((lst)->data != nullptr) {                                                          \
                r_free((lst)->data);                                                               \
                (lst)->data = nullptr;                                                             \
            }                                                                                      \
                                                                                                   \
            (lst)->size = 0;                                                                       \
            (lst)->capacity = 0;                                                                   \
        }                                                                                          \
    })

#define list_type_size(lst) sizeof(typeof_unqual(*(lst)->data))

#define list_get(lst, idx)                                                                         \
    ({                                                                                             \
        assert((lst) != nullptr);                                                                  \
        assert((lst)->data != nullptr);                                                            \
        assert((idx) < (lst)->size);                                                               \
        (lst)->data[(idx)];                                                                        \
    })

#define list_resize(lst, new_capacity)                                                             \
    FUNC({                                                                                         \
        assert((lst) != nullptr);                                                                  \
        assert((lst)->data != nullptr);                                                            \
        assert((new_capacity) >= (lst)->size);                                                     \
                                                                                                   \
        void * data = r_recalloc((lst)->data, new_capacity, list_type_size(lst));                  \
        (lst)->capacity = new_capacity;                                                            \
        (lst)->data = data;                                                                        \
    })

#define list_grow(lst)                                                                             \
    FUNC({                                                                                         \
        if ((lst)->capacity <= 0) {                                                                \
            (lst)->capacity = 4;                                                                   \
            (lst)->data = r_calloc((lst)->capacity, list_type_size(lst));                          \
        }                                                                                          \
                                                                                                   \
        while ((lst)->size >= (lst)->capacity) {                                                   \
            list_resize((lst), (lst)->capacity << 1);                                              \
        }                                                                                          \
    })

#define list_shrink(lst)                                                                           \
    FUNC({                                                                                         \
        assert((lst) != nullptr);                                                                  \
                                                                                                   \
        while ((lst)->size < (lst)->capacity >> 2) {                                               \
            list_resize((lst), (lst)->capacity >> 1);                                              \
        }                                                                                          \
    })

#define list_add(lst, item)                                                                        \
    FUNC({                                                                                         \
        assert((lst) != nullptr);                                                                  \
                                                                                                   \
        const size_t idx = (lst)->size;                                                            \
        (lst)->size++;                                                                             \
        list_grow((lst));                                                                          \
                                                                                                   \
        (lst)->data[idx] = (item);                                                                 \
    })

#define list_insert(lst, idx, item)                                                                \
    FUNC({                                                                                         \
        assert((lst) != nullptr);                                                                  \
        assert((lst)->data != nullptr);                                                            \
        assert((idx) <= (lst)->size);                                                              \
                                                                                                   \
        (lst)->size++;                                                                             \
        list_grow((lst));                                                                          \
                                                                                                   \
        const size_t tail = (lst)->size - (idx);                                                   \
        if (tail > 0) {                                                                            \
            void * dst = &((lst)->data)[(idx) + 1];                                                \
            void * src = &((lst)->data)[(idx)];                                                    \
            memmove(dst, src, tail * sizeof(typeof_unqual(item)));                                 \
        }                                                                                          \
                                                                                                   \
        (lst)->data[(idx)] = (item);                                                               \
    })

#define list_remove(lst, idx)                                                                      \
    ({                                                                                             \
        assert((lst) != nullptr);                                                                  \
        assert((lst)->data != nullptr);                                                            \
        assert((idx) < (lst)->size);                                                               \
                                                                                                   \
        auto removed = (lst)->data[(idx)];                                                         \
                                                                                                   \
        const size_t tail = (lst)->size - (idx) - 1;                                               \
        if (tail > 0) {                                                                            \
            void * dst = &((lst)->data)[(idx)];                                                    \
            void * src = &((lst)->data)[(idx) + 1];                                                \
            memmove(dst, src, tail * list_type_size(lst));                                         \
        }                                                                                          \
                                                                                                   \
        (lst)->size--;                                                                             \
        list_shrink((lst));                                                                        \
                                                                                                   \
        removed;                                                                                   \
    })

#endif // RUNE_LIST_API

#ifdef T

typedef struct {
    T * data;
    size_t size;
    size_t capacity;
} LIST(T);

static LIST(T) R_LIST_OF(T)(const T first, ...) {
    LIST(T)
    lst = {
        .data = nullptr,
        .size = 0,
        .capacity = 0,
    };

    va_list args;
    va_start(args, first);
    T next = first;
    while (next != R_END) {
        list_add(&lst, next);
        next = va_arg(args, T);
    }
    va_end(args);

    return lst;
}

#endif // T

// ---------------
// lock-free queue
// ---------------

#ifndef RUNE_LFQ_API
#define RUNE_LFQ_API

#define LFQ(type) GLUE(lfq_, type)
#define R_LFQ_OF(type) GLUE(LFQ(type), _of)

#define lfq(type, cap, ...) R_LFQ_OF(type)((cap)VA_ARGS(__VA_ARGS__), R_END)

#define lfq_free(q)                                                                                \
    FUNC({                                                                                         \
        if ((q) != nullptr) {                                                                      \
            if ((q)->data != nullptr) {                                                            \
                r_free((q)->data);                                                                 \
                (q)->data = nullptr;                                                               \
            }                                                                                      \
                                                                                                   \
            (q)->capacity = 0;                                                                     \
            (q)->head = 0;                                                                         \
            (q)->tail = 0;                                                                         \
        }                                                                                          \
    })

#define lfq_type_size(q) sizeof(typeof_unqual(*(q)->data))

#define lfq_capacity(q) (q)->capacity

#define lfq_depth(q)                                                                               \
    ({                                                                                             \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        (atomic_load(&(q)->tail) - atomic_load(&(q)->head)) % (q)->capacity;                       \
    })

#define lfq_empty(q)                                                                               \
    ({                                                                                             \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        atomic_load(&(q)->head) == atomic_load(&(q)->tail);                                        \
    })

#define lfq_full(q)                                                                                \
    ({                                                                                             \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        (atomic_load(&(q)->tail) + 1) % (q)->capacity == atomic_load(&(q)->head);                  \
    })

#define lfq_peek(q)                                                                                \
    ({                                                                                             \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        const size_t head = atomic_load(&(q)->head);                                               \
                                                                                                   \
        auto result = R_END;                                                                       \
        if (head != atomic_load(&(q)->tail)) {                                                     \
            result = (q)->data[head];                                                              \
        }                                                                                          \
        result;                                                                                    \
    })

#define lfq_clear(q)                                                                               \
    FUNC({                                                                                         \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        atomic_store(&(q)->head, 0);                                                               \
        atomic_store(&(q)->tail, 0);                                                               \
    })

#define lfq_push(q, item)                                                                          \
    FUNC({                                                                                         \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        const size_t tail = atomic_load(&(q)->tail);                                               \
        const size_t next_tail = (tail + 1) % (q)->capacity;                                       \
                                                                                                   \
        auto result = item;                                                                        \
        if (next_tail == atomic_load(&(q)->head)) {                                                \
            result = R_END;                                                                        \
        } else {                                                                                   \
            (q)->data[tail] = (item);                                                              \
            atomic_store(&(q)->tail, next_tail);                                                   \
        }                                                                                          \
        result;                                                                                    \
    })

#define lfq_pop(q)                                                                                 \
    ({                                                                                             \
        assert((q) != nullptr);                                                                    \
                                                                                                   \
        const size_t head = atomic_load(&(q)->head);                                               \
                                                                                                   \
        auto item = R_END;                                                                         \
        if (head != atomic_load(&(q)->tail)) {                                                     \
            item = (q)->data[head];                                                                \
            atomic_store(&(q)->head, (head + 1) % (q)->capacity);                                  \
        }                                                                                          \
        item;                                                                                      \
    })

#define lfq_resize(q, new_capacity)                                                                \
    FUNC({                                                                                         \
        assert((q) != nullptr);                                                                    \
        assert((new_capacity) >= (q)->capacity);                                                   \
                                                                                                   \
        const size_t old_capacity = (q)->capacity;                                                 \
        const size_t head = (q)->head;                                                             \
        const size_t tail = (q)->tail;                                                             \
        const size_t type_size = lfq_type_size(q);                                                 \
                                                                                                   \
        /* Resize the existing memory block */                                                     \
        void * new_data = r_recalloc((q)->data, new_capacity, type_size);                          \
                                                                                                   \
        if (tail < head) {                                                                         \
            /* Handle wrap-around by moving the second part of the data */                         \
            const size_t first = old_capacity - head;                                              \
            memmove((char *)new_data + first * type_size, new_data, tail * type_size);             \
        }                                                                                          \
                                                                                                   \
        (q)->data = new_data;                                                                      \
        (q)->capacity = new_capacity;                                                              \
        (q)->head = 0;                                                                             \
        (q)->tail = (tail >= head) ? (tail - head) : (old_capacity - head + tail);                 \
    })

#endif // RUNE_LFQ_API

#ifdef T

#ifdef _Atomic
#define R_Atomic(type) _Atomic(type)
#else // Fallback to a simple type if _Atomic is not available
#define R_Atomic(type) type
#endif // _Atomic

typedef struct {
    T * data;
    size_t capacity;
    R_Atomic(size_t) head;
    R_Atomic(size_t) tail;
} LFQ(T);

static LFQ(T) R_LFQ_OF(T)(const size_t capacity, ...) {
    LFQ(T)
    q = {
        .data = r_calloc_t(capacity, T),
        .capacity = capacity,
        .head = 0,
        .tail = 0,
    };

    va_list args;
    va_start(args, capacity);
    T value;
    while ((value = va_arg(args, T)) != R_END) {
        lfq_push(&q, value);
    }
    va_end(args);

    return q;
}

#endif // T
