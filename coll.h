//
// Created by Matt on 4/27/2025.
//

// ReSharper disable once CppMissingIncludeGuard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "std.h"

// ----
// LIST
// ----

#ifndef RUNE_LIST_API
#define RUNE_LIST_API

#define RUNE_LIST_TYPE(type) GLUE(list_, type)
#define RUNE_LIST_OF(type) GLUE(RUNE_LIST_TYPE, _of)

#define list(type, ...)                                                                            \
    ({                                                                                             \
        RUNE_LIST_TYPE(type)                                                                       \
        lst = {                                                                                    \
            .data = NULL,                                                                          \
            .type_size = sizeof(type),                                                             \
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
            (lst)->type_size = 0;                                                                  \
            (lst)->size = 0;                                                                       \
            (lst)->capacity = 0;                                                                   \
        }                                                                                          \
    })

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
        void * data = r_recalloc((lst)->data, new_capacity, (lst)->type_size);                     \
        (lst)->capacity = new_capacity;                                                            \
        (lst)->data = data;                                                                        \
    })

#define list_grow(lst)                                                                             \
    FUNC({                                                                                         \
        if ((lst)->capacity <= 0) {                                                                \
            (lst)->capacity = 4;                                                                   \
            (lst)->data = r_calloc((lst)->capacity, (lst)->type_size);                             \
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
            memmove(dst, src, tail * (lst)->type_size);                                            \
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
            memmove(dst, src, tail * (lst)->type_size);                                            \
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
    size_t type_size;
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
