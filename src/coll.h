//
// Created by Matt on 4/27/2025.
//

// ReSharper disable once CppMissingIncludeGuard
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "std.h"

// ----
// list
// ----

#ifndef RUNE_LIST_API
#define RUNE_LIST_API

#define RUNE_LIST_TYPE(type) GLUE(list_, type)
#define RUNE_LIST_OF(type) GLUE(RUNE_LIST_TYPE(type), _of)

#define list(type, ...)                                                                            \
    ({                                                                                             \
        RUNE_LIST_TYPE(type)                                                                       \
        lst = {                                                                                    \
            .data = NULL,                                                                          \
            .size = 0,                                                                             \
            .capacity = 0,                                                                         \
        };                                                                                         \
        RUNE_LIST_OF(type)(&lst VA_ARGS(__VA_ARGS__), R_END);                                      \
        lst;                                                                                       \
    })

#define list_free(lst)                                                                             \
    FUNC({                                                                                         \
        if ((lst) != NULL) {                                                                       \
            if ((lst)->data != NULL) {                                                             \
                r_free((lst)->data);                                                               \
                (lst)->data = NULL;                                                                \
            }                                                                                      \
                                                                                                   \
            (lst)->size = 0;                                                                       \
            (lst)->capacity = 0;                                                                   \
        }                                                                                          \
    })

#define list_type_size(lst) sizeof(typeof((lst)->data[0]))

#define list_get(lst, idx)                                                                         \
    ({                                                                                             \
        assert((lst) != NULL);                                                                     \
        assert((lst)->data != NULL);                                                               \
        assert((idx) < (lst)->size);                                                               \
        (lst)->data[(idx)];                                                                        \
    })

#define list_resize(lst, new_capacity)                                                             \
    FUNC({                                                                                         \
        assert((lst) != NULL);                                                                     \
        assert((lst)->data != NULL);                                                               \
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
        assert((lst) != NULL);                                                                     \
                                                                                                   \
        while ((lst)->size < (lst)->capacity >> 2) {                                               \
            list_resize((lst), (lst)->capacity >> 1);                                              \
        }                                                                                          \
    })

#define list_add(lst, item)                                                                        \
    FUNC({                                                                                         \
        assert((lst) != NULL);                                                                     \
                                                                                                   \
        const size_t idx = (lst)->size;                                                            \
        (lst)->size++;                                                                             \
        list_grow((lst));                                                                          \
                                                                                                   \
        (lst)->data[idx] = (item);                                                                 \
    })

#define list_insert(lst, idx, item)                                                                \
    FUNC({                                                                                         \
        assert((lst) != NULL);                                                                     \
        assert((lst)->data != NULL);                                                               \
        assert((idx) <= (lst)->size);                                                              \
                                                                                                   \
        (lst)->size++;                                                                             \
        list_grow((lst));                                                                          \
                                                                                                   \
        const size_t tail = (lst)->size - (idx);                                                   \
        if (tail > 0) {                                                                            \
            void * dst = &((lst)->data)[(idx) + 1];                                                \
            void * src = &((lst)->data)[(idx)];                                                    \
            memmove(dst, src, tail * sizeof(typeof(item)));                                        \
        }                                                                                          \
                                                                                                   \
        (lst)->data[(idx)] = (item);                                                               \
    })

#define list_remove(lst, idx)                                                                      \
    ({                                                                                             \
        assert((lst) != NULL);                                                                     \
        assert((lst)->data != NULL);                                                               \
        assert((idx) < (lst)->size);                                                               \
                                                                                                   \
        const typeof((lst)->data[0]) removed = (lst)->data[(idx)];                                 \
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

#define LIST RUNE_LIST_TYPE(T)
typedef struct LIST {
    T * data;
    size_t size;
    size_t capacity;
} LIST;

#define LIST_OF RUNE_LIST_OF(T)
static void LIST_OF(LIST * lst, ...) {
    assert(lst != NULL);

    va_list args;
    va_start(args, lst);
    T value;
    while ((value = va_arg(args, T)) != R_END) {
        list_add(lst, value);
    }
    va_end(args);
}
#undef LIST_OF

#undef LIST

#endif // T

// ---------------
// lock-free queue
// ---------------

#ifndef RUNE_LFQ_API
#define RUNE_LFQ_API

#define RUNE_LFQ_TYPE(type) GLUE(lfq_, type)
#define RUNE_LFQ_OF(type) GLUE(RUNE_LFQ_TYPE(type), _of)

#define lfq(type, cap, ...)                                                                        \
    ({                                                                                             \
        RUNE_LFQ_TYPE(type)                                                                        \
        q = {                                                                                      \
            .data = r_calloc_t(cap, type),                                                         \
            .capacity = (cap),                                                                     \
            .head = 0,                                                                             \
            .tail = 0,                                                                             \
        };                                                                                         \
        RUNE_LFQ_OF(type)(&q VA_ARGS(__VA_ARGS__), R_END);                                         \
        q;                                                                                         \
    })

#define lfq_free(q)                                                                                \
    FUNC({                                                                                         \
        if ((q) != NULL) {                                                                         \
            if ((q)->data != NULL) {                                                               \
                r_free((q)->data);                                                                 \
                (q)->data = NULL;                                                                  \
            }                                                                                      \
                                                                                                   \
            (q)->capacity = 0;                                                                     \
            (q)->head = 0;                                                                         \
            (q)->tail = 0;                                                                         \
        }                                                                                          \
    })

#define lfq_type_size(q) sizeof(typeof((q)->data[0]))

#define lfq_capacity(q) (q)->capacity

#define lfq_depth(q)                                                                               \
    ({                                                                                             \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        (atomic_load(&(q)->tail) - atomic_load(&(q)->head)) % (q)->capacity;                       \
    })

#define lfq_empty(q)                                                                               \
    ({                                                                                             \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        atomic_load(&(q)->head) == atomic_load(&(q)->tail);                                        \
    })

#define lfq_full(q)                                                                                \
    ({                                                                                             \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        (atomic_load(&(q)->tail) + 1) % (q)->capacity == atomic_load(&(q)->head);                  \
    })

#define lfq_peek(q)                                                                                \
    ({                                                                                             \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        const size_t head = atomic_load(&(q)->head);                                               \
                                                                                                   \
        typeof((q)->data[0]) result = R_END;                                                       \
        if (head != atomic_load(&(q)->tail)) {                                                     \
            result = (q)->data[head];                                                              \
        }                                                                                          \
        result;                                                                                    \
    })

#define lfq_clear(q)                                                                               \
    FUNC({                                                                                         \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        atomic_store(&(q)->head, 0);                                                               \
        atomic_store(&(q)->tail, 0);                                                               \
    })

#define lfq_push(q, item)                                                                          \
    FUNC({                                                                                         \
        assert((q) != NULL);                                                                       \
                                                                                                   \
        const size_t tail = atomic_load(&(q)->tail);                                               \
        const size_t next_tail = (tail + 1) % (q)->capacity;                                       \
                                                                                                   \
        typeof((q)->data[0]) result = item;                                                        \
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
        assert((q) != NULL);                                                                       \
                                                                                                   \
        const size_t head = atomic_load(&(q)->head);                                               \
                                                                                                   \
        typeof((q)->data[0]) item = R_END;                                                         \
        if (head != atomic_load(&(q)->tail)) {                                                     \
            item = (q)->data[head];                                                                \
            atomic_store(&(q)->head, (head + 1) % (q)->capacity);                                  \
        }                                                                                          \
        item;                                                                                      \
    })

#define lfq_resize(q, new_capacity)                                                                \
    FUNC({                                                                                         \
        assert((q) != NULL);                                                                       \
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

#define LFQ RUNE_LFQ_TYPE(T)

#ifdef _Atomic
#define R_Atomic(type) _Atomic(type)
#else
#define R_Atomic(type) type
#endif

typedef struct LFQ {
    T * data;
    size_t capacity;
    R_Atomic(size_t) head;
    R_Atomic(size_t) tail;
} LFQ;

#define LFQ_OF RUNE_LFQ_OF(T)
static void LFQ_OF(LFQ * q, ...) {
    assert(q != NULL);

    va_list args;
    va_start(args, q);
    T value;
    while ((value = va_arg(args, T)) != R_END) {
        lfq_push(q, value);
    }
    va_end(args);
}
#undef LFQ_OF

#undef LFQ

#endif // T
