/*
 * coll tests.
 */

// ReSharper disable CppDFATimeOver
#include "../src/coll.h"
#include "CUnit/Basic.h"
#include "test.h"

#include <stdlib.h>

// =====================================================================================================================
// LIST TESTS
// =====================================================================================================================

// Define LIST(int) for testing
#define T int
#include "../src/coll.h"

#include <limits.h>
#undef T

// =====================================================================================================================
// list() - Create list
// =====================================================================================================================

static void list__for_single_element__should_return__list_with_size_one(void) {
    LIST(int) lst = list(int, 42);
    CU_ASSERT_EQUAL(lst.size, 1);
    CU_ASSERT_EQUAL(lst.capacity, 4); // Initial capacity
    CU_ASSERT_PTR_NOT_NULL(lst.data);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 42);
    list_free(&lst);
}

static void list__for_multiple_elements__should_return__list_with_all_elements(void) {
    LIST(int) lst = list(int, 1, 2, 3, 4, 5);
    CU_ASSERT_EQUAL(lst.size, 5);
    CU_ASSERT_PTR_NOT_NULL(lst.data);

    for (int i = 0; i < 5; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i + 1);
    }
    list_free(&lst);
}

static void list__for_empty__should_return__list_with_zero_size(void) {
    LIST(int) lst = list(int);
    CU_ASSERT_EQUAL(lst.size, 0);
    CU_ASSERT_EQUAL(lst.capacity, 0);
    CU_ASSERT_PTR_NULL(lst.data);
    list_free(&lst);
}

// =====================================================================================================================
// list_add() - Add element to list
// =====================================================================================================================

static void list_add__for_single_item__should_add_to_empty_list(void) {
    LIST(int) lst = list(int);
    CU_ASSERT_EQUAL(lst.size, 0);

    list_add(&lst, 42);
    CU_ASSERT_EQUAL(lst.size, 1);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 42);
    list_free(&lst);
}

static void list_add__for_multiple_items__should_grow_list(void) {
    LIST(int) lst = list(int, 1);
    CU_ASSERT_EQUAL(lst.size, 1);

    for (int i = 2; i <= 10; i++) {
        list_add(&lst, i);
    }
    CU_ASSERT_EQUAL(lst.size, 10);

    for (int i = 0; i < 10; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i + 1);
    }
    list_free(&lst);
}

static void list_add__for_many_items__should_trigger_multiple_growths(void) {
    LIST(int) lst = list(int);
    const size_t initial_capacity = lst.capacity;

    for (int i = 0; i < 100; i++) {
        list_add(&lst, i);
    }

    CU_ASSERT_EQUAL(lst.size, 100);
    CU_ASSERT(lst.capacity > initial_capacity);

    for (int i = 0; i < 100; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i);
    }
    list_free(&lst);
}

// =====================================================================================================================
// list_insert() - Insert element at index
// =====================================================================================================================

static void list_insert__at_beginning__should_shift_elements(void) {
    LIST(int) lst = list(int, 1, 2, 3);
    CU_ASSERT_EQUAL(lst.size, 3);

    list_insert(&lst, 0, 99);
    CU_ASSERT_EQUAL(lst.size, 4);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 99);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 3), 3);
    list_free(&lst);
}

static void list_insert__at_middle__should_shift_elements(void) {
    LIST(int) lst = list(int, 1, 2, 3);
    list_insert(&lst, 1, 99);

    CU_ASSERT_EQUAL(lst.size, 4);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 99);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 3), 3);
    list_free(&lst);
}

static void list_insert__at_end__should_append_element(void) {
    LIST(int) lst = list(int, 1, 2, 3);
    list_insert(&lst, 3, 99);

    CU_ASSERT_EQUAL(lst.size, 4);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 3);
    CU_ASSERT_EQUAL(list_get(&lst, 3), 99);
    list_free(&lst);
}

// =====================================================================================================================
// list_remove() - Remove element at index
// =====================================================================================================================

static void list_remove__from_beginning__should_shift_elements(void) {
    LIST(int) lst = list(int, 1, 2, 3, 4);
    const int removed = list_remove(&lst, 0);

    CU_ASSERT_EQUAL(removed, 1);
    CU_ASSERT_EQUAL(lst.size, 3);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 3);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 4);
    list_free(&lst);
}

static void list_remove__from_middle__should_shift_elements(void) {
    LIST(int) lst = list(int, 1, 2, 3, 4);
    const int removed = list_remove(&lst, 1);

    CU_ASSERT_EQUAL(removed, 2);
    CU_ASSERT_EQUAL(lst.size, 3);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 3);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 4);
    list_free(&lst);
}

static void list_remove__from_end__should_remove_last_element(void) {
    LIST(int) lst = list(int, 1, 2, 3, 4);
    const int removed = list_remove(&lst, 3);

    CU_ASSERT_EQUAL(removed, 4);
    CU_ASSERT_EQUAL(lst.size, 3);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 3);
    list_free(&lst);
}

static void list_remove__all_elements__should_empty_list(void) {
    LIST(int) lst = list(int, 1, 2, 3);

    list_remove(&lst, 0);
    CU_ASSERT_EQUAL(lst.size, 2);

    list_remove(&lst, 0);
    CU_ASSERT_EQUAL(lst.size, 1);

    list_remove(&lst, 0);
    CU_ASSERT_EQUAL(lst.size, 0);
    list_free(&lst);
}

// =====================================================================================================================
// list_resize() - Resize list capacity
// =====================================================================================================================

static void list_resize__to_larger_capacity__should_grow_list(void) {
    LIST(int) lst = list(int, 1, 2, 3);

    list_resize(&lst, 16);
    CU_ASSERT_EQUAL(lst.capacity, 16);
    CU_ASSERT_EQUAL(lst.size, 3); // Size unchanged

    for (int i = 0; i < 3; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i + 1);
    }
    list_free(&lst);
}

static void list_resize__to_smaller_capacity__should_shrink_list(void) {
    LIST(int) lst = list(int);
    lst.capacity = 16;
    lst.data = mem_alloc_zero(16 * sizeof(int));
    lst.size = 4;

    for (int i = 0; i < 4; i++) {
        lst.data[i] = i + 1;
    }

    list_resize(&lst, 8);
    CU_ASSERT_EQUAL(lst.capacity, 8);
    CU_ASSERT_EQUAL(lst.size, 4);

    for (int i = 0; i < 4; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i + 1);
    }
    list_free(&lst);
}

// =====================================================================================================================
// list_grow() - Grow list capacity
// =====================================================================================================================

static void list_grow__on_empty_list__should_allocate_initial_capacity(void) {
    LIST(int) lst = list(int);
    CU_ASSERT_EQUAL(lst.capacity, 0);
    CU_ASSERT_PTR_NULL(lst.data);

    list_grow(&lst);
    CU_ASSERT_EQUAL(lst.capacity, 4);
    CU_ASSERT_PTR_NOT_NULL(lst.data);
    list_free(&lst);
}

static void list_grow__when_full__should_double_capacity(void) {
    LIST(int) lst = list(int, 1, 2, 3);
    const size_t old_capacity = lst.capacity;

    list_add(&lst, 4); // This triggers grow
    CU_ASSERT_EQUAL(lst.capacity, old_capacity << 1);
    CU_ASSERT_EQUAL(lst.size, 4);
    list_free(&lst);
}

// =====================================================================================================================
// list_free() - Free list memory
// =====================================================================================================================

static void list_free__on_list__should_deallocate_and_reset(void) {
    LIST(int) lst = list(int, 1, 2, 3);
    const void * old_data = lst.data;
    CU_ASSERT_PTR_NOT_NULL(old_data);

    list_free(&lst);
    CU_ASSERT_PTR_NULL(lst.data);
    CU_ASSERT_EQUAL(lst.size, 0);
    CU_ASSERT_EQUAL(lst.capacity, 0);
}

static void list_free__on_empty_list__should_be_safe(void) {
    LIST(int) lst = list(int);
    list_free(&lst); // Should not crash

    CU_ASSERT_PTR_NULL(lst.data);
    CU_ASSERT_EQUAL(lst.size, 0);
    CU_ASSERT_EQUAL(lst.capacity, 0);
}

// =====================================================================================================================
// lfq() - Create lock-free queue
// =====================================================================================================================

static void lfq__for_capacity__should_return__initialized_queue(void) {
    LFQ(int) q = lfq(int, 10);
    CU_ASSERT_EQUAL(q.capacity, 10);
    CU_ASSERT_EQUAL(atomic_load(&q.head), 0);
    CU_ASSERT_EQUAL(atomic_load(&q.tail), 0);
    CU_ASSERT_PTR_NOT_NULL(q.data);
    lfq_free(&q);
}

static void lfq__with_initial_elements__should_add_all_elements(void) {
    LFQ(int) q = lfq(int, 10, 1, 2, 3, 4, 5);
    CU_ASSERT_EQUAL(q.capacity, 10);
    CU_ASSERT_EQUAL(lfq_depth(&q), 5);
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_push() / lfq_pop() - Queue push and pop operations
// =====================================================================================================================

static void lfq_push__single_item__should_add_to_queue(void) {
    LFQ(int) q = lfq(int, 10);
    const int result = lfq_push(&q, 42);

    CU_ASSERT_EQUAL(result, 42);
    CU_ASSERT_EQUAL(lfq_depth(&q), 1);
    lfq_free(&q);
}

static void lfq_push__multiple_items__should_add_all(void) {
    LFQ(int) q = lfq(int, 10);

    for (int i = 1; i <= 5; i++) {
        const int result = lfq_push(&q, i);
        CU_ASSERT_EQUAL(result, i);
        CU_ASSERT_EQUAL(lfq_depth(&q), i);
    }
    lfq_free(&q);
}

static void lfq_push__when_full__should_return__end_marker(void) {
    LFQ(int) q = lfq(int, 3); // Capacity 3, so max 2 items

    lfq_push(&q, 1);
    lfq_push(&q, 2);
    const int result = lfq_push(&q, 3); // Queue full

    CU_ASSERT_EQUAL(result, (typeof(int)){0});
    CU_ASSERT_EQUAL(lfq_depth(&q), 2);
    lfq_free(&q);
}

static void lfq_pop__single_item__should_remove_from_queue(void) {
    LFQ(int) q = lfq(int, 10, 42);
    CU_ASSERT_EQUAL(lfq_depth(&q), 1);

    const int value = lfq_pop(&q);
    CU_ASSERT_EQUAL(value, 42);
    CU_ASSERT_EQUAL(lfq_depth(&q), 0);
    lfq_free(&q);
}

static void lfq_pop__multiple_items__should_fifo_order(void) {
    LFQ(int) q = lfq(int, 10, 1, 2, 3, 4, 5);

    for (int i = 1; i <= 5; i++) {
        const int value = lfq_pop(&q);
        CU_ASSERT_EQUAL(value, i);
    }
    CU_ASSERT_EQUAL(lfq_depth(&q), 0);
    lfq_free(&q);
}

static void lfq_pop__when_empty__should_return__end_marker(void) {
    LFQ(int) q = lfq(int, 10);
    const int value = lfq_pop(&q);

    CU_ASSERT_EQUAL(value, (typeof(int)){0});
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_peek() - Peek at queue front
// =====================================================================================================================

static void lfq_peek__with_item__should_return__item_without_removing(void) {
    LFQ(int) q = lfq(int, 10, 42);

    const int peek_value = lfq_peek(&q);
    CU_ASSERT_EQUAL(peek_value, 42);
    CU_ASSERT_EQUAL(lfq_depth(&q), 1); // Still 1

    const int pop_value = lfq_pop(&q);
    CU_ASSERT_EQUAL(pop_value, 42);
    lfq_free(&q);
}

static void lfq_peek__when_empty__should_return__end_marker(void) {
    LFQ(int) q = lfq(int, 10);
    const int value = lfq_peek(&q);

    CU_ASSERT_EQUAL(value, (typeof(int)){0});
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_empty() / lfq_full() - Queue empty and full checks
// =====================================================================================================================

static void lfq_empty__on_new_queue__should_return__true(void) {
    LFQ(int) q = lfq(int, 10);
    CU_ASSERT_TRUE(lfq_empty(&q));
    lfq_free(&q);
}

static void lfq_empty__with_item__should_return__false(void) {
    LFQ(int) q = lfq(int, 10, 42);
    CU_ASSERT_FALSE(lfq_empty(&q));
    lfq_free(&q);
}

static void lfq_full__with_space__should_return__false(void) {
    LFQ(int) q = lfq(int, 10, 1, 2);
    CU_ASSERT_FALSE(lfq_full(&q));
    lfq_free(&q);
}

static void lfq_full__when_full__should_return__true(void) {
    LFQ(int) q = lfq(int, 3); // Capacity 3, max 2 items
    lfq_push(&q, 1);
    lfq_push(&q, 2);

    CU_ASSERT_TRUE(lfq_full(&q));
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_clear() - Clear queue
// =====================================================================================================================

static void lfq_clear__on_queue_with_items__should_reset_pointers(void) {
    LFQ(int) q = lfq(int, 10, 1, 2, 3);
    CU_ASSERT_EQUAL(lfq_depth(&q), 3);

    lfq_clear(&q);
    CU_ASSERT_EQUAL(atomic_load(&q.head), 0);
    CU_ASSERT_EQUAL(atomic_load(&q.tail), 0);
    CU_ASSERT_EQUAL(lfq_depth(&q), 0);
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_resize() - Resize queue capacity
// =====================================================================================================================

static void lfq_resize__to_larger_capacity__should_grow_queue(void) {
    LFQ(int) q = lfq(int, 5, 1, 2, 3);

    lfq_resize(&q, 10);
    CU_ASSERT_EQUAL(q.capacity, 10);
    CU_ASSERT_EQUAL(lfq_depth(&q), 3);
    lfq_free(&q);
}

// =====================================================================================================================
// lfq_free() - Free queue memory
// =====================================================================================================================

static void lfq_free__on_queue__should_deallocate_and_reset(void) {
    LFQ(int) q = lfq(int, 10, 1, 2, 3);
    const void * old_data = q.data;
    CU_ASSERT_PTR_NOT_NULL(old_data);

    lfq_free(&q);
    CU_ASSERT_PTR_NULL(q.data);
    CU_ASSERT_EQUAL(q.capacity, 0);
}

// =====================================================================================================================
// COMPLEX TYPE TESTS
// =====================================================================================================================

// Test with a struct type
typedef struct {
    int x;
    int y;
} Point;

#define T Point
#include "../src/coll.h"
#undef T

static void list__for_struct_type__should_work_correctly(void) {
    LIST(Point) lst = list(Point);
    list_add(&lst, ((Point){.x = 1, .y = 2}));
    list_add(&lst, ((Point){.x = 3, .y = 4}));

    const Point p1 = list_get(&lst, 0);
    const Point p2 = list_get(&lst, 1);

    CU_ASSERT_EQUAL(p1.x, 1);
    CU_ASSERT_EQUAL(p1.y, 2);
    CU_ASSERT_EQUAL(p2.x, 3);
    CU_ASSERT_EQUAL(p2.y, 4);
    list_free(&lst);
}

// =====================================================================================================================
// Stress tests
// =====================================================================================================================

static void list__for_1000_adds__should_succeed(void) {
    LIST(int) lst = list(int);

    for (int i = 0; i < 1000; i++) {
        list_add(&lst, i);
    }

    CU_ASSERT_EQUAL(lst.size, 1000);

    for (int i = 0; i < 1000; i++) {
        CU_ASSERT_EQUAL(list_get(&lst, i), i);
    }
    list_free(&lst);
}

static void list__for_alternating_add_remove__should_maintain_order(void) {
    LIST(int) lst = list(int, 1, 2, 3, 4, 5);

    // Remove middle, add at end
    list_remove(&lst, 2);
    list_add(&lst, 6);

    CU_ASSERT_EQUAL(lst.size, 5);
    CU_ASSERT_EQUAL(list_get(&lst, 0), 1);
    CU_ASSERT_EQUAL(list_get(&lst, 1), 2);
    CU_ASSERT_EQUAL(list_get(&lst, 2), 4);
    CU_ASSERT_EQUAL(list_get(&lst, 3), 5);
    CU_ASSERT_EQUAL(list_get(&lst, 4), 6);
    list_free(&lst);
}

static void lfq__for_push_pop_cycles__should_maintain_fifo(void) {
    LFQ(int) q = lfq(int, 10);

    // Push 5, pop 3
    for (int i = 1; i <= 5; i++) {
        lfq_push(&q, i);
    }

    for (int i = 1; i <= 3; i++) {
        const int v = lfq_pop(&q);
        CU_ASSERT_EQUAL(v, i);
    }

    CU_ASSERT_EQUAL(lfq_depth(&q), 2);

    // Push 3 more
    for (int i = 6; i <= 8; i++) {
        lfq_push(&q, i);
    }

    CU_ASSERT_EQUAL(lfq_depth(&q), 5);

    // Pop all remaining
    for (int i = 4; i <= 8; i++) {
        const int v = lfq_pop(&q);
        CU_ASSERT_EQUAL(v, i);
    }

    CU_ASSERT_EQUAL(lfq_depth(&q), 0);
    lfq_free(&q);
}

// =====================================================================================================================
// Test suite registration
// =====================================================================================================================

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // list() suite
    CU_pSuite suite_list = CU_add_suite("list()", nullptr, nullptr);
    if (suite_list == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list, list__for_single_element__should_return__list_with_size_one);
    ADD_TEST(suite_list, list__for_multiple_elements__should_return__list_with_all_elements);
    ADD_TEST(suite_list, list__for_empty__should_return__list_with_zero_size);
    ADD_TEST(suite_list, list__for_struct_type__should_work_correctly);

    // list_add() suite
    CU_pSuite suite_list_add = CU_add_suite("list_add()", nullptr, nullptr);
    if (suite_list_add == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_add, list_add__for_single_item__should_add_to_empty_list);
    ADD_TEST(suite_list_add, list_add__for_multiple_items__should_grow_list);
    ADD_TEST(suite_list_add, list_add__for_many_items__should_trigger_multiple_growths);

    // list_insert() suite
    CU_pSuite suite_list_insert = CU_add_suite("list_insert()", nullptr, nullptr);
    if (suite_list_insert == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_insert, list_insert__at_beginning__should_shift_elements);
    ADD_TEST(suite_list_insert, list_insert__at_middle__should_shift_elements);
    ADD_TEST(suite_list_insert, list_insert__at_end__should_append_element);

    // list_remove() suite
    CU_pSuite suite_list_remove = CU_add_suite("list_remove()", nullptr, nullptr);
    if (suite_list_remove == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_remove, list_remove__from_beginning__should_shift_elements);
    ADD_TEST(suite_list_remove, list_remove__from_middle__should_shift_elements);
    ADD_TEST(suite_list_remove, list_remove__from_end__should_remove_last_element);
    ADD_TEST(suite_list_remove, list_remove__all_elements__should_empty_list);

    // list_resize() suite
    CU_pSuite suite_list_resize = CU_add_suite("list_resize()", nullptr, nullptr);
    if (suite_list_resize == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_resize, list_resize__to_larger_capacity__should_grow_list);
    ADD_TEST(suite_list_resize, list_resize__to_smaller_capacity__should_shrink_list);

    // list_grow() suite
    CU_pSuite suite_list_grow = CU_add_suite("list_grow()", nullptr, nullptr);
    if (suite_list_grow == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_grow, list_grow__on_empty_list__should_allocate_initial_capacity);
    ADD_TEST(suite_list_grow, list_grow__when_full__should_double_capacity);

    // list_free() suite
    CU_pSuite suite_list_free = CU_add_suite("list_free()", nullptr, nullptr);
    if (suite_list_free == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_list_free, list_free__on_list__should_deallocate_and_reset);
    ADD_TEST(suite_list_free, list_free__on_empty_list__should_be_safe);

    // lfq() suite
    CU_pSuite suite_lfq = CU_add_suite("lfq()", nullptr, nullptr);
    if (suite_lfq == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq, lfq__for_capacity__should_return__initialized_queue);
    ADD_TEST(suite_lfq, lfq__with_initial_elements__should_add_all_elements);

    // lfq_push() / lfq_pop() suite
    CU_pSuite suite_lfq_push_pop = CU_add_suite("lfq_push() / lfq_pop()", nullptr, nullptr);
    if (suite_lfq_push_pop == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_push_pop, lfq_push__single_item__should_add_to_queue);
    ADD_TEST(suite_lfq_push_pop, lfq_push__multiple_items__should_add_all);
    ADD_TEST(suite_lfq_push_pop, lfq_push__when_full__should_return__end_marker);
    ADD_TEST(suite_lfq_push_pop, lfq_pop__single_item__should_remove_from_queue);
    ADD_TEST(suite_lfq_push_pop, lfq_pop__multiple_items__should_fifo_order);
    ADD_TEST(suite_lfq_push_pop, lfq_pop__when_empty__should_return__end_marker);

    // lfq_peek() suite
    CU_pSuite suite_lfq_peek = CU_add_suite("lfq_peek()", nullptr, nullptr);
    if (suite_lfq_peek == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_peek, lfq_peek__with_item__should_return__item_without_removing);
    ADD_TEST(suite_lfq_peek, lfq_peek__when_empty__should_return__end_marker);

    // lfq_empty() / lfq_full() suite
    CU_pSuite suite_lfq_empty_full = CU_add_suite("lfq_empty() / lfq_full()", nullptr, nullptr);
    if (suite_lfq_empty_full == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_empty_full, lfq_empty__on_new_queue__should_return__true);
    ADD_TEST(suite_lfq_empty_full, lfq_empty__with_item__should_return__false);
    ADD_TEST(suite_lfq_empty_full, lfq_full__with_space__should_return__false);
    ADD_TEST(suite_lfq_empty_full, lfq_full__when_full__should_return__true);

    // lfq_clear() suite
    CU_pSuite suite_lfq_clear = CU_add_suite("lfq_clear()", nullptr, nullptr);
    if (suite_lfq_clear == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_clear, lfq_clear__on_queue_with_items__should_reset_pointers);

    // lfq_resize() suite
    CU_pSuite suite_lfq_resize = CU_add_suite("lfq_resize()", nullptr, nullptr);
    if (suite_lfq_resize == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_resize, lfq_resize__to_larger_capacity__should_grow_queue);

    // lfq_free() suite
    CU_pSuite suite_lfq_free = CU_add_suite("lfq_free()", nullptr, nullptr);
    if (suite_lfq_free == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_lfq_free, lfq_free__on_queue__should_deallocate_and_reset);

    // Stress tests suite
    CU_pSuite suite_stress = CU_add_suite("Stress tests", nullptr, nullptr);
    if (suite_stress == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_stress, list__for_1000_adds__should_succeed);
    ADD_TEST(suite_stress, list__for_alternating_add_remove__should_maintain_order);
    ADD_TEST(suite_stress, lfq__for_push_pop_cycles__should_maintain_fifo);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
