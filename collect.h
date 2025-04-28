//
// Created by Matt on 4/27/2025.
//

// ReSharper disable once CppMissingIncludeGuard
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "std.h"

// ---------------
// LIST definition
// ---------------

#ifndef T_NAME
#define T_NAME T
#define T_NAME_DEFAULTED
#endif

#define LIST GLUE(list_, T_NAME)

typedef struct LIST {
    T * data;
    size_t type_size;
    size_t size;
    size_t capacity;
} LIST;

#undef LIST

#ifdef T_NAME_DEFAULTED
#undef T_NAME
#undef T_NAME_DEFAULTED
#endif

#ifndef LIST_API
#define LIST_API

#define list_of(type)                                                          \
    {                                                                          \
        .data = NULL,                                                          \
        .type_size = sizeof(type),                                             \
        .size = 0,                                                             \
        .capacity = 0,                                                         \
    }

#define list_free(lst)                                                         \
    FUNC({                                                                     \
        if (lst != NULL) {                                                     \
            if ((lst)->data != NULL) {                                         \
                r_free((lst)->data);                                           \
                (lst)->data = NULL;                                            \
            }                                                                  \
                                                                               \
            (lst)->type_size = 0;                                              \
            (lst)->size = 0;                                                   \
            (lst)->capacity = 0;                                               \
        }                                                                      \
    })

#define list_resize(lst, new_capacity)                                         \
    FUNC({                                                                     \
        assert((lst) != NULL);                                                 \
        assert(new_capacity >= (lst)->size);                                   \
                                                                               \
        void * data = r_recalloc((lst)->data, new_capacity, (lst)->type_size); \
        (lst)->capacity = new_capacity;                                        \
        (lst)->data = data;                                                    \
    })

#define list_grow(lst)                                                         \
    FUNC({                                                                     \
        if (lst->capacity <= 0) {                                              \
            lst->capacity = 4;                                                 \
            lst->data = r_calloc(lst->capacity, lst->type_size);               \
        }                                                                      \
                                                                               \
        while ((lst)->size >= (lst)->capacity) {                               \
            list_resize((lst), (lst)->capacity << 1);                          \
        }                                                                      \
    })

#define list_add(lst, item)                                                    \
    FUNC({                                                                     \
        assert((lst) != NULL);                                                 \
                                                                               \
        const size_t idx = (lst)->size;                                        \
        (lst)->size++;                                                         \
        list_grow((lst));                                                      \
                                                                               \
        (lst)->data[idx] = (item);                                             \
    })

#define list_insert(lst, idx, item)                                            \
    FUNC({                                                                     \
        assert((lst) != NULL);                                                 \
        assert((idx) <= (lst)->size);                                          \
                                                                               \
        (lst)->size++;                                                         \
        list_grow((lst));                                                      \
                                                                               \
        const size_t tail = (lst)->size - (idx);                               \
        if (tail > 0) {                                                        \
            void * dst = &((lst)->data)[(idx) + 1];                            \
            void * src = &((lst)->data)[(idx)];                                \
            memmove(dst, src, tail * (lst)->type_size);                        \
        }                                                                      \
                                                                               \
        (lst)->data[(idx)] = (item);                                           \
    })
#endif // LIST_API
