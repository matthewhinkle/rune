#if defined(__cplusplus)
#error "This file must be compiled as C, not C++"
#endif // no cpp assert

#include <stdio.h>

#define T float
#include "coll.h"
#undef T

int main() {
    printf("rune\n");

    LIST(float) my_list = list(float, 1.0f, 2.0f, 3.0f);
    list_free(&my_list);

    return 0;
}
