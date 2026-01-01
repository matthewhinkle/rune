/*
 * tree tests.
 */

// ReSharper disable CppDFATimeOver
// Suppress variable shadowing warnings from macro expansions (rbt_remove, etc.)
#pragma GCC diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wshadow"

#include "../src/tree.h"
#include "CUnit/Basic.h"
#include "test.h"

#include <limits.h>

// Define RBT(int) for testing
#define T int
#include "../src/tree.h"
#undef T

// =====================================================================================================================
// RBT helper functions
// =====================================================================================================================

// Count nodes in a subtree
static size_t rbt_tree_count_nodes(const struct RBT_NODE(int) * node) {
    if (node == nullptr) {
        return 0;
    }
    return 1 + rbt_tree_count_nodes(node->left) + rbt_tree_count_nodes(node->right);
}

// Check if node value exists in tree
static bool rbt_tree_contains(const struct RBT_NODE(int) * node, const int val) {
    if (node == nullptr) {
        return false;
    }
    if (node->data == val) {
        return true;
    }
    if (val < node->data) {
        return rbt_tree_contains(node->left, val);
    }
    return rbt_tree_contains(node->right, val);
}

// Free entire tree recursively
static void rbt_tree_free_tree(struct RBT_NODE(int) * node) {
    if (node == nullptr) {
        return;
    }
    rbt_tree_free_tree(node->left);
    rbt_tree_free_tree(node->right);
    mem_free(node, sizeof(struct RBT_NODE(int)));
}

// Check if node is red
static bool rbt_tree_is_red(const struct RBT_NODE(int) * node) {
    return node != nullptr && node->color == R_(rbt_red);
}

// Check if node is black (or null)
[[maybe_unused]] static bool rbt_tree_is_black(const struct RBT_NODE(int) * node) {
    return node == nullptr || node->color == R_(rbt_black);
}

// Check red-black property: no red node has a red child
static bool rbt_tree_check_no_red_red(const struct RBT_NODE(int) * node) {
    if (node == nullptr) {
        return true;
    }
    if (rbt_tree_is_red(node)) {
        if (rbt_tree_is_red(node->left) || rbt_tree_is_red(node->right)) {
            return false;
        }
    }
    return rbt_tree_check_no_red_red(node->left) && rbt_tree_check_no_red_red(node->right);
}

// Calculate black height - should be same for all paths
static int rbt_tree_black_height(const struct RBT_NODE(int) * node) {
    if (node == nullptr) {
        return 1; // null nodes count as black
    }
    int left_height = rbt_tree_black_height(node->left);
    int right_height = rbt_tree_black_height(node->right);

    if (left_height == -1 || right_height == -1 || left_height != right_height) {
        return -1; // invalid tree
    }

    return left_height + (node->color == R_(rbt_black) ? 1 : 0);
}

// Check if tree maintains BST property
static bool rbt_tree_check_bst(const struct RBT_NODE(int) * node, int min_val, int max_val) {
    if (node == nullptr) {
        return true;
    }
    if (node->data <= min_val || node->data >= max_val) {
        return false;
    }
    return rbt_tree_check_bst(node->left, min_val, node->data) && rbt_tree_check_bst(node->right, node->data, max_val);
}

// =====================================================================================================================
// BST API tests
// =====================================================================================================================

static void bst_min__for_single_node__should_return_node(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 42);

    // Act
    auto min_node = bst_min(tree.root);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(min_node);
    CU_ASSERT_EQUAL(min_node->data, 42);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_min__for_left_skewed_tree__should_return_leftmost_node(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 6, 18};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act
    auto min_node = bst_min(tree.root);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(min_node);
    CU_ASSERT_EQUAL(min_node->data, 6);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_min__for_subtree__should_return_minimum_in_subtree(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act - find minimum in right subtree
    auto right_subtree = tree.root->right;
    auto min_node = bst_min(right_subtree);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(min_node);
    CU_ASSERT_EQUAL(min_node->data, 62);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_find__for_empty_tree__should_return_null(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act
    auto found = bst_find(&tree, 42);

    // Assert
    CU_ASSERT_PTR_NULL(found);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_find__for_existing_value__should_return_node(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        auto found = bst_find(&tree, values[i]);
        CU_ASSERT_PTR_NOT_NULL(found);
        CU_ASSERT_EQUAL(found->data, values[i]);
    }

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_find__for_non_existing_value__should_return_null(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert
    CU_ASSERT_PTR_NULL(bst_find(&tree, 1));
    CU_ASSERT_PTR_NULL(bst_find(&tree, 100));
    CU_ASSERT_PTR_NULL(bst_find(&tree, 42));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_remove__for_leaf_node__should_remove_and_update_parent(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);
    rbt_insert(&tree, 25);
    rbt_insert(&tree, 75);

    // Act - remove leaf node
    auto node_to_remove = bst_find(&tree, 25);
    CU_ASSERT_PTR_NOT_NULL(node_to_remove);
    bst_remove(&tree, node_to_remove);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 25));
    CU_ASSERT_TRUE(rbt_contains(&tree, 50));
    CU_ASSERT_TRUE(rbt_contains(&tree, 75));
    CU_ASSERT_TRUE(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_remove__for_node_with_one_child__should_promote_child(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);
    rbt_insert(&tree, 25);
    rbt_insert(&tree, 75);
    rbt_insert(&tree, 12); // Left child of 25

    // Act - remove node with one child
    auto node_to_remove = bst_find(&tree, 25);
    CU_ASSERT_PTR_NOT_NULL(node_to_remove);
    bst_remove(&tree, node_to_remove);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 25));
    CU_ASSERT_TRUE(rbt_contains(&tree, 50));
    CU_ASSERT_TRUE(rbt_contains(&tree, 75));
    CU_ASSERT_TRUE(rbt_contains(&tree, 12));
    CU_ASSERT_TRUE(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_remove__for_node_with_two_children__should_replace_with_successor(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act - remove node with two children
    auto node_to_remove = bst_find(&tree, 50);
    CU_ASSERT_PTR_NOT_NULL(node_to_remove);
    bst_remove(&tree, node_to_remove);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 50));
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        if (values[i] != 50) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
        }
    }
    CU_ASSERT_TRUE(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_remove__for_root_node_with_two_children__should_maintain_tree_structure(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Save root value before removal (root may not be 50 due to RBT rebalancing)
    int root_value = tree.root->data;

    // Act - remove root
    bst_remove(&tree, tree.root);

    // Assert - tree should still be valid and contain all values except the removed root
    CU_ASSERT_FALSE(rbt_contains(&tree, root_value));
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        int val = values[i];
        if (val != root_value) {
            CU_ASSERT_TRUE(rbt_contains(&tree, val));
        }
    }
    CU_ASSERT_TRUE(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void bst_remove__for_sequential_removal__should_maintain_bst_property(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - remove each value one by one
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        auto node_to_remove = bst_find(&tree, values[i]);
        CU_ASSERT_PTR_NOT_NULL(node_to_remove);
        bst_remove(&tree, node_to_remove);

        // Verify remaining values still in tree
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[j]));
        }

        // Verify removed value is gone
        CU_ASSERT_FALSE(rbt_contains(&tree, values[i]));

        // Verify BST property is maintained
        if (tree.root != nullptr) {
            CU_ASSERT_TRUE(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));
        }
    }

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// RBT contains tests
// =====================================================================================================================

static void rbt_contains__for_empty_tree__should_return_false(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act & Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 42));
    CU_ASSERT_FALSE(rbt_contains(&tree, 0));
    CU_ASSERT_FALSE(rbt_contains(&tree, -100));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_single_element_tree__should_find_element(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);

    // Act & Assert
    CU_ASSERT_TRUE(rbt_contains(&tree, 50));
    CU_ASSERT_FALSE(rbt_contains(&tree, 49));
    CU_ASSERT_FALSE(rbt_contains(&tree, 51));
    CU_ASSERT_FALSE(rbt_contains(&tree, 0));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_multiple_elements__should_find_all(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    size_t count = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - all inserted values should be found
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
    }

    // Non-existent values should not be found
    CU_ASSERT_FALSE(rbt_contains(&tree, 1));
    CU_ASSERT_FALSE(rbt_contains(&tree, 100));
    CU_ASSERT_FALSE(rbt_contains(&tree, 50000));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_left_skewed_tree__should_find_elements(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Insert values that create left-skewed structure (before rebalancing)
    for (int i = 5; i >= 1; i--) {
        rbt_insert(&tree, i);
    }

    // Act & Assert - all inserted values should be found
    for (int i = 1; i <= 5; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    // Non-existent values
    CU_ASSERT_FALSE(rbt_contains(&tree, 0));
    CU_ASSERT_FALSE(rbt_contains(&tree, 6));
    CU_ASSERT_FALSE(rbt_contains(&tree, 10));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_right_skewed_tree__should_find_elements(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Insert values that create right-skewed structure (before rebalancing)
    for (int i = 1; i <= 5; i++) {
        rbt_insert(&tree, i);
    }

    // Act & Assert - all inserted values should be found
    for (int i = 1; i <= 5; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    // Non-existent values
    CU_ASSERT_FALSE(rbt_contains(&tree, 0));
    CU_ASSERT_FALSE(rbt_contains(&tree, 6));
    CU_ASSERT_FALSE(rbt_contains(&tree, 100));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_balanced_tree__should_find_all_elements(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {20, 10, 30, 5, 15, 25, 35, 2, 7, 12, 18};
    size_t count = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - all inserted values should be found
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
    }

    // Non-existent values in various ranges
    CU_ASSERT_FALSE(rbt_contains(&tree, 1));
    CU_ASSERT_FALSE(rbt_contains(&tree, 3));
    CU_ASSERT_FALSE(rbt_contains(&tree, 6));
    CU_ASSERT_FALSE(rbt_contains(&tree, 8));
    CU_ASSERT_FALSE(rbt_contains(&tree, 40));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_negative_values__should_find_elements(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {-10, -50, 0, 30, -100, 50};
    size_t count = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - all inserted values should be found
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
    }

    // Non-existent negative values
    CU_ASSERT_FALSE(rbt_contains(&tree, -1));
    CU_ASSERT_FALSE(rbt_contains(&tree, -25));
    CU_ASSERT_FALSE(rbt_contains(&tree, -101));
    CU_ASSERT_FALSE(rbt_contains(&tree, 1));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_large_tree__should_find_all_values(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Insert values in random-ish order to trigger rebalancing
    int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43, 56, 68, 81, 93};
    for (int i = 0; i < 15; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Insert remaining numbers
    for (int i = 1; i <= 100; i++) {
        bool already_inserted = false;
        for (int j = 0; j < 15; j++) {
            if (values[j] == i) {
                already_inserted = true;
                break;
            }
        }
        if (!already_inserted) {
            rbt_insert(&tree, i);
        }
    }

    // Act & Assert - all values 1-100 should be found
    for (int i = 1; i <= 100; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    // Values outside range should not be found
    CU_ASSERT_FALSE(rbt_contains(&tree, 0));
    CU_ASSERT_FALSE(rbt_contains(&tree, 101));
    CU_ASSERT_FALSE(rbt_contains(&tree, 200));
    CU_ASSERT_FALSE(rbt_contains(&tree, -1));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_duplicate_insertion__should_still_find_value(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 42);
    rbt_insert(&tree, 42); // Attempt duplicate
    rbt_insert(&tree, 42); // Another duplicate

    // Act & Assert
    CU_ASSERT_TRUE(rbt_contains(&tree, 42));
    CU_ASSERT_FALSE(rbt_contains(&tree, 41));
    CU_ASSERT_FALSE(rbt_contains(&tree, 43));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_contains__for_boundary_values__should_find_extremes(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, INT_MIN);
    rbt_insert(&tree, INT_MAX);
    rbt_insert(&tree, 0);
    rbt_insert(&tree, -1);
    rbt_insert(&tree, 1);

    // Act & Assert
    CU_ASSERT_TRUE(rbt_contains(&tree, INT_MIN));
    CU_ASSERT_TRUE(rbt_contains(&tree, INT_MAX));
    CU_ASSERT_TRUE(rbt_contains(&tree, 0));
    CU_ASSERT_TRUE(rbt_contains(&tree, -1));
    CU_ASSERT_TRUE(rbt_contains(&tree, 1));

    // Non-existent boundaries
    CU_ASSERT_FALSE(rbt_contains(&tree, INT_MIN + 1));
    CU_ASSERT_FALSE(rbt_contains(&tree, INT_MAX - 1));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// RBT insertion tests
// =====================================================================================================================

static void rbt_insert__for_empty_tree__should_create_root_node(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->data, 50);
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));
    CU_ASSERT_PTR_NULL(tree.root->parent);
    CU_ASSERT_PTR_NULL(tree.root->left);
    CU_ASSERT_PTR_NULL(tree.root->right);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_simple_sequence__should_create_valid_tree(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 10);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 15);

    // Assert
    // Root should be black
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));
    CU_ASSERT_EQUAL(tree.root->data, 10);

    // Left child should be red
    CU_ASSERT_PTR_NOT_NULL(tree.root->left);
    CU_ASSERT_EQUAL(tree.root->left->data, 5);
    CU_ASSERT_EQUAL(tree.root->left->color, R_(rbt_red));

    // Right child should be red
    CU_ASSERT_PTR_NOT_NULL(tree.root->right);
    CU_ASSERT_EQUAL(tree.root->right->data, 15);
    CU_ASSERT_EQUAL(tree.root->right->color, R_(rbt_red));

    // All nodes should have correct parent pointers
    CU_ASSERT_EQUAL(tree.root->left->parent->data, 10);
    CU_ASSERT_EQUAL(tree.root->right->parent->data, 10);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_duplicate_value__should_ignore(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);
    const size_t initial_count = rbt_tree_count_nodes(tree.root);

    // Act
    rbt_insert(&tree, 50);

    // Assert
    const size_t final_count = rbt_tree_count_nodes(tree.root);
    CU_ASSERT_EQUAL(initial_count, final_count);
    CU_ASSERT_EQUAL(tree.root->data, 50);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_red_uncle__should_recolor_and_propagate(void) {
    // Arrange
    // Building: Insert 50, 30, 70, 20
    // After 50, 30, 70: tree has 50(B), 30(R), 70(R)
    // Uncle of 20 is 70, which is red -> triggers recolor
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 50);
    rbt_insert(&tree, 30);
    rbt_insert(&tree, 70);
    rbt_insert(&tree, 20);

    // Assert
    // All nodes should exist
    CU_ASSERT(rbt_tree_contains(tree.root, 50));
    CU_ASSERT(rbt_tree_contains(tree.root, 30));
    CU_ASSERT(rbt_tree_contains(tree.root, 70));
    CU_ASSERT(rbt_tree_contains(tree.root, 20));

    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // After recoloring, 30 and 70 should be black, root stays black
    CU_ASSERT_EQUAL(tree.root->left->color, R_(rbt_black));
    CU_ASSERT_EQUAL(tree.root->right->color, R_(rbt_black));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_left_left_case__should_maintain_invariants(void) {
    // Arrange
    // Insert 30, 20, 10 (ascending insertion that triggers left-left)
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 30);
    rbt_insert(&tree, 20);
    rbt_insert(&tree, 10);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->data, 20);             // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black)); // root must be black

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));            // no red-red violations
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);          // valid black height
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX)); // maintains BST

    // All values present
    CU_ASSERT(rbt_tree_contains(tree.root, 10));
    CU_ASSERT(rbt_tree_contains(tree.root, 20));
    CU_ASSERT(rbt_tree_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 3);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_left_right_case__should_maintain_invariants(void) {
    // Arrange
    // Insert 10, 30, 20 (creates left-right scenario)
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 10);
    rbt_insert(&tree, 30);
    rbt_insert(&tree, 20);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->data, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rbt_tree_contains(tree.root, 10));
    CU_ASSERT(rbt_tree_contains(tree.root, 20));
    CU_ASSERT(rbt_tree_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 3);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_right_right_case__should_maintain_invariants(void) {
    // Arrange
    // Insert 10, 20, 30 (ascending insertion that triggers right-right)
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 10);
    rbt_insert(&tree, 20);
    rbt_insert(&tree, 30);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->data, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rbt_tree_contains(tree.root, 10));
    CU_ASSERT(rbt_tree_contains(tree.root, 20));
    CU_ASSERT(rbt_tree_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 3);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_right_left_case__should_maintain_invariants(void) {
    // Arrange
    // Insert 30, 10, 20 (creates right-left scenario)
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 30);
    rbt_insert(&tree, 10);
    rbt_insert(&tree, 20);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->data, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rbt_tree_contains(tree.root, 10));
    CU_ASSERT(rbt_tree_contains(tree.root, 20));
    CU_ASSERT(rbt_tree_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 3);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_seven_node_tree__should_maintain_all_invariants(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act - insert 1-7
    for (int i = 1; i <= 7; i++) {
        rbt_insert(&tree, i);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (int i = 1; i <= 7; i++) {
        CU_ASSERT(rbt_tree_contains(tree.root, i));
    }
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 7);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_descending_ten_nodes__should_maintain_all_invariants(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act - insert 10 down to 1 (worst case for unbalanced insertion)
    for (int i = 10; i >= 1; i--) {
        rbt_insert(&tree, i);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (int i = 1; i <= 10; i++) {
        CU_ASSERT(rbt_tree_contains(tree.root, i));
    }
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 10);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_insert__for_mixed_insertion_pattern__should_maintain_all_invariants(void) {
    // Arrange
    // Insert in pattern: 50, 25, 75, 12, 37, 62, 87, 6, 18, 31
    RBT(int) tree = rbt(int);
    const int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31};
    constexpr size_t count = sizeof(values) / sizeof(values[0]);

    // Act
    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, R_(rbt_black));

    // RB-tree invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT(rbt_tree_contains(tree.root, values[i]));
    }
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), count);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// RBT removal tests
// =====================================================================================================================

static void rbt_remove__for_empty_tree__should_handle_gracefully(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act
    rbt_remove(&tree, 42);

    // Assert - tree should remain empty
    CU_ASSERT_PTR_NULL(tree.root);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_single_node_root__should_delete_node(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);

    // Act
    rbt_remove(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NULL(tree.root);
    CU_ASSERT_FALSE(rbt_contains(&tree, 50));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_red_leaf__should_maintain_invariants(void) {
    // Arrange - Build tree where we know a node will be red
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);
    rbt_insert(&tree, 25);
    rbt_insert(&tree, 75);
    // Both 25 and 75 are red children of black root

    // Act - Remove a red leaf
    rbt_remove(&tree, 25);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 25));
    CU_ASSERT_TRUE(rbt_contains(&tree, 50));
    CU_ASSERT_TRUE(rbt_contains(&tree, 75));
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_black_leaf__should_handle_double_black(void) {
    // Arrange - Build tree and identify a black leaf to remove
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act - Remove a black node (leaf after structure)
    rbt_remove(&tree, 12);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 12));
    // Check all values except the removed one (12)
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        if (values[i] != 12) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
        }
    }
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_node_with_one_child__should_replace_with_child(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 50);
    rbt_insert(&tree, 25);
    rbt_insert(&tree, 75);
    rbt_insert(&tree, 12);
    rbt_insert(&tree, 37);

    // Act - Remove node 25 which has two children (or one after structure)
    size_t removed_count_before = rbt_tree_count_nodes(tree.root);
    rbt_remove(&tree, 25);
    size_t removed_count_after = rbt_tree_count_nodes(tree.root);

    // Assert
    CU_ASSERT_EQUAL(removed_count_after, removed_count_before - 1);
    CU_ASSERT_FALSE(rbt_contains(&tree, 25));
    CU_ASSERT_TRUE(rbt_contains(&tree, 50));
    CU_ASSERT_TRUE(rbt_contains(&tree, 75));
    CU_ASSERT_TRUE(rbt_contains(&tree, 12));
    CU_ASSERT_TRUE(rbt_contains(&tree, 37));
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_node_with_two_children__should_replace_with_successor(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act - Remove node with two children (e.g., 25 or 75)
    rbt_remove(&tree, 50);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, 50));
    for (size_t i = 1; i < sizeof(values) / sizeof(values[0]); i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
    }
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_sequential_single_deletions__should_maintain_invariants(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - Remove each value one by one
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_remove(&tree, values[i]);

        // Verify removed value is gone
        CU_ASSERT_FALSE(rbt_contains(&tree, values[i]));

        // Verify remaining values are still present
        for (size_t j = i + 1; j < sizeof(values) / sizeof(values[0]); j++) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[j]));
        }

        // Verify invariants
        if (tree.root != nullptr) {
            CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
            CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
            CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));
        }
    }

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_reverse_sequential_deletions__should_maintain_invariants(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - Remove in reverse order
    for (int i = (int)sizeof(values) / (int)sizeof(values[0]) - 1; i >= 0; i--) {
        rbt_remove(&tree, values[i]);

        // Verify removed value is gone
        CU_ASSERT_FALSE(rbt_contains(&tree, values[i]));

        // Verify remaining values
        for (int j = 0; j < i; j++) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[j]));
        }

        // Verify invariants
        if (tree.root != nullptr) {
            CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
            CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
            CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));
        }
    }

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_deletion_from_larger_tree__should_maintain_all_invariants(void) {
    // Arrange - Insert 1-20
    RBT(int) tree = rbt(int);
    for (int i = 1; i <= 20; i++) {
        rbt_insert(&tree, i);
    }

    // Act - Remove odd numbers
    for (int i = 1; i <= 20; i += 2) {
        rbt_remove(&tree, i);
    }

    // Assert - Only even numbers should remain
    for (int i = 1; i <= 20; i++) {
        if (i % 2 == 0) {
            CU_ASSERT_TRUE(rbt_contains(&tree, i));
        } else {
            CU_ASSERT_FALSE(rbt_contains(&tree, i));
        }
    }

    // Check all invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));
    CU_ASSERT_EQUAL(rbt_tree_count_nodes(tree.root), 10);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_removing_every_other_element__should_maintain_balance(void) {
    // Arrange - Build a balanced tree with many nodes
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43, 56, 68, 81, 93};
    for (int i = 0; i < 15; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act - Remove alternate values
    for (int i = 0; i < 15; i += 2) {
        rbt_remove(&tree, values[i]);
    }

    // Assert
    int final_height = rbt_tree_black_height(tree.root);
    // Black height should be reasonable (not degenerate)
    CU_ASSERT(final_height > 0);
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_deletion_of_root__should_promote_successor(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        rbt_insert(&tree, values[i]);
    }

    int root_before = tree.root->data;

    // Act - Remove current root
    rbt_remove(&tree, root_before);

    // Assert
    CU_ASSERT_FALSE(rbt_contains(&tree, root_before));
    CU_ASSERT_PTR_NOT_NULL(tree.root); // Tree should still have a root (if elements remain)
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // All other values should still be present
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        if (values[i] != root_before) {
            CU_ASSERT_TRUE(rbt_contains(&tree, values[i]));
        }
    }

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_remove__for_stress_test_insert_remove_patterns__should_maintain_invariants(void) {
    // Arrange & Act - Interleave inserts and removes
    RBT(int) tree = rbt(int);

    // Insert 1-50
    for (int i = 1; i <= 50; i++) {
        rbt_insert(&tree, i);
    }

    // Remove 1, 3, 5, 7, ... (odd numbers up to 25)
    for (int i = 1; i <= 25; i += 2) {
        rbt_remove(&tree, i);
    }

    // Insert 51-60
    for (int i = 51; i <= 60; i++) {
        rbt_insert(&tree, i);
    }

    // Remove 26, 28, 30, ... (even numbers from 26-50)
    for (int i = 26; i <= 50; i += 2) {
        rbt_remove(&tree, i);
    }

    // Assert - Check remaining values
    for (int i = 1; i <= 25; i++) {
        if (i % 2 == 0) {
            CU_ASSERT_TRUE(rbt_contains(&tree, i));
        } else {
            CU_ASSERT_FALSE(rbt_contains(&tree, i));
        }
    }

    for (int i = 26; i <= 50; i++) {
        if (i % 2 != 0) {
            CU_ASSERT_TRUE(rbt_contains(&tree, i));
        } else {
            CU_ASSERT_FALSE(rbt_contains(&tree, i));
        }
    }

    for (int i = 51; i <= 60; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    // Check invariants
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);
    CU_ASSERT(rbt_tree_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// RBT SIZE FIELD TRACKING TESTS
// =====================================================================================================================

static void rbt_size__for_empty_tree__should_be_zero(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act & Assert
    CU_ASSERT_EQUAL(tree.size, 0);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_single_insert__should_be_one(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act
    rbt_insert(&tree, 42);

    // Assert
    CU_ASSERT_EQUAL(tree.size, 1);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_multiple_inserts__should_match_node_count(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    size_t count = sizeof(values) / sizeof(values[0]);

    // Act
    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Assert
    CU_ASSERT_EQUAL(tree.size, count);
    CU_ASSERT_EQUAL(tree.size, rbt_tree_count_nodes(tree.root));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_duplicate_insert__should_not_change(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 42);
    size_t size_before = tree.size;

    // Act
    rbt_insert(&tree, 42);
    rbt_insert(&tree, 42);

    // Assert
    CU_ASSERT_EQUAL(tree.size, size_before);
    CU_ASSERT_EQUAL(tree.size, 1);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_single_remove__should_decrement(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 42);
    CU_ASSERT_EQUAL(tree.size, 1);

    // Act
    rbt_remove(&tree, 42);

    // Assert
    CU_ASSERT_EQUAL(tree.size, 0);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_multiple_removes__should_match_remaining_count(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    size_t count = sizeof(values) / sizeof(values[0]);
    for (size_t i = 0; i < count; i++) {
        rbt_insert(&tree, values[i]);
    }

    // Act & Assert - Remove each value and check size
    for (size_t i = 0; i < count; i++) {
        CU_ASSERT_EQUAL(tree.size, count - i);
        rbt_remove(&tree, values[i]);
        CU_ASSERT_EQUAL(tree.size, count - i - 1);
    }

    CU_ASSERT_EQUAL(tree.size, 0);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_sequential_operations__should_track_correctly(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act & Assert - Insert 1-10
    for (int i = 1; i <= 10; i++) {
        rbt_insert(&tree, i);
        CU_ASSERT_EQUAL(tree.size, (size_t)i);
    }

    // Remove 1, 3, 5, 7, 9 (odd numbers)
    for (int i = 1; i <= 10; i += 2) {
        size_t size_before = tree.size;
        rbt_remove(&tree, i);
        CU_ASSERT_EQUAL(tree.size, size_before - 1);
    }

    // Should have 5 elements left (2, 4, 6, 8, 10)
    CU_ASSERT_EQUAL(tree.size, 5);
    CU_ASSERT_EQUAL(tree.size, rbt_tree_count_nodes(tree.root));

    // Insert 11-15
    for (int i = 11; i <= 15; i++) {
        rbt_insert(&tree, i);
    }

    CU_ASSERT_EQUAL(tree.size, 10);
    CU_ASSERT_EQUAL(tree.size, rbt_tree_count_nodes(tree.root));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_stress_test__should_remain_consistent(void) {
    // Arrange
    RBT(int) tree = rbt(int);

    // Act - Insert 1-20
    for (int i = 1; i <= 20; i++) {
        rbt_insert(&tree, i);
    }
    CU_ASSERT_EQUAL(tree.size, 20);

    // Remove odd numbers (1, 3, 5, ... 19)
    for (int i = 1; i <= 20; i += 2) {
        rbt_remove(&tree, i);
    }
    CU_ASSERT_EQUAL(tree.size, 10);

    // Insert 21-25
    for (int i = 21; i <= 25; i++) {
        rbt_insert(&tree, i);
    }
    CU_ASSERT_EQUAL(tree.size, 15);

    // Verify even numbers 2-20 remain
    for (int i = 2; i <= 20; i += 2) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    // Verify 21-25 exist
    for (int i = 21; i <= 25; i++) {
        CU_ASSERT_TRUE(rbt_contains(&tree, i));
    }

    CU_ASSERT_EQUAL(tree.size, rbt_tree_count_nodes(tree.root));

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

static void rbt_size__after_removing_nonexistent_value__should_not_change(void) {
    // Arrange
    RBT(int) tree = rbt(int);
    rbt_insert(&tree, 42);
    size_t size_before = tree.size;

    // Act
    rbt_remove(&tree, 99); // Value not in tree

    // Assert
    CU_ASSERT_EQUAL(tree.size, size_before);
    CU_ASSERT_EQUAL(tree.size, 1);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// RBT WITH CUSTOM COMPARATOR TESTS
// =====================================================================================================================

// Custom comparator for reverse order (descending)
static int rbt_tree_cmp_reverse(int a, int b) {
    return (b > a) - (b < a); // Reverse of standard comparison
}

static void rbt__with_custom_comparator__should_use_custom_comparison(void) {
    // Create tree (comparator passed per-operation now)
    RBT(int) tree = rbt(int);

    // Insert values with custom comparator
    rbt_insert(&tree, 5, rbt_tree_cmp_reverse);
    rbt_insert(&tree, 3, rbt_tree_cmp_reverse);
    rbt_insert(&tree, 7, rbt_tree_cmp_reverse);
    rbt_insert(&tree, 1, rbt_tree_cmp_reverse);
    rbt_insert(&tree, 9, rbt_tree_cmp_reverse);

    // All values should be found (using same comparator)
    CU_ASSERT_TRUE(rbt_contains(&tree, 1, rbt_tree_cmp_reverse));
    CU_ASSERT_TRUE(rbt_contains(&tree, 3, rbt_tree_cmp_reverse));
    CU_ASSERT_TRUE(rbt_contains(&tree, 5, rbt_tree_cmp_reverse));
    CU_ASSERT_TRUE(rbt_contains(&tree, 7, rbt_tree_cmp_reverse));
    CU_ASSERT_TRUE(rbt_contains(&tree, 9, rbt_tree_cmp_reverse));

    // Values not inserted should not be found
    CU_ASSERT_FALSE(rbt_contains(&tree, 2, rbt_tree_cmp_reverse));
    CU_ASSERT_FALSE(rbt_contains(&tree, 4, rbt_tree_cmp_reverse));
    CU_ASSERT_FALSE(rbt_contains(&tree, 6, rbt_tree_cmp_reverse));

    // Check tree structure is valid despite custom comparator
    CU_ASSERT(rbt_tree_check_no_red_red(tree.root));
    CU_ASSERT(rbt_tree_black_height(tree.root) != -1);

    // Cleanup
    rbt_tree_free_tree(tree.root);
}

// =====================================================================================================================
// Test suite registration
// =====================================================================================================================

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // bst_min() suite
    CU_pSuite suite_bst_min = CU_add_suite("bst_min()", nullptr, nullptr);
    if (suite_bst_min == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_bst_min, bst_min__for_single_node__should_return_node);
    ADD_TEST(suite_bst_min, bst_min__for_left_skewed_tree__should_return_leftmost_node);
    ADD_TEST(suite_bst_min, bst_min__for_subtree__should_return_minimum_in_subtree);

    // bst_find() suite
    CU_pSuite suite_bst_find = CU_add_suite("bst_find()", nullptr, nullptr);
    if (suite_bst_find == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_bst_find, bst_find__for_empty_tree__should_return_null);
    ADD_TEST(suite_bst_find, bst_find__for_existing_value__should_return_node);
    ADD_TEST(suite_bst_find, bst_find__for_non_existing_value__should_return_null);

    // bst_remove() suite
    CU_pSuite suite_bst_remove = CU_add_suite("bst_remove()", nullptr, nullptr);
    if (suite_bst_remove == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_bst_remove, bst_remove__for_leaf_node__should_remove_and_update_parent);
    ADD_TEST(suite_bst_remove, bst_remove__for_node_with_one_child__should_promote_child);
    ADD_TEST(suite_bst_remove, bst_remove__for_node_with_two_children__should_replace_with_successor);
    ADD_TEST(suite_bst_remove, bst_remove__for_root_node_with_two_children__should_maintain_tree_structure);
    ADD_TEST(suite_bst_remove, bst_remove__for_sequential_removal__should_maintain_bst_property);

    // rbt_contains() suite
    CU_pSuite suite_rbt_contains = CU_add_suite("rbt_contains()", nullptr, nullptr);
    if (suite_rbt_contains == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_rbt_contains, rbt_contains__for_empty_tree__should_return_false);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_single_element_tree__should_find_element);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_multiple_elements__should_find_all);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_left_skewed_tree__should_find_elements);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_right_skewed_tree__should_find_elements);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_balanced_tree__should_find_all_elements);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_negative_values__should_find_elements);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_large_tree__should_find_all_values);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_duplicate_insertion__should_still_find_value);
    ADD_TEST(suite_rbt_contains, rbt_contains__for_boundary_values__should_find_extremes);

    // rbt_insert() suite
    CU_pSuite suite_rbt_insert = CU_add_suite("rbt_insert()", nullptr, nullptr);
    if (suite_rbt_insert == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_rbt_insert, rbt_insert__for_empty_tree__should_create_root_node);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_simple_sequence__should_create_valid_tree);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_duplicate_value__should_ignore);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_red_uncle__should_recolor_and_propagate);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_left_left_case__should_maintain_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_left_right_case__should_maintain_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_right_right_case__should_maintain_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_right_left_case__should_maintain_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_seven_node_tree__should_maintain_all_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_descending_ten_nodes__should_maintain_all_invariants);
    ADD_TEST(suite_rbt_insert, rbt_insert__for_mixed_insertion_pattern__should_maintain_all_invariants);

    // rbt_remove() suite
    CU_pSuite suite_rbt_remove = CU_add_suite("rbt_remove()", nullptr, nullptr);
    if (suite_rbt_remove == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_rbt_remove, rbt_remove__for_empty_tree__should_handle_gracefully);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_single_node_root__should_delete_node);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_red_leaf__should_maintain_invariants);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_black_leaf__should_handle_double_black);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_node_with_one_child__should_replace_with_child);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_node_with_two_children__should_replace_with_successor);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_sequential_single_deletions__should_maintain_invariants);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_reverse_sequential_deletions__should_maintain_invariants);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_deletion_from_larger_tree__should_maintain_all_invariants);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_removing_every_other_element__should_maintain_balance);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_deletion_of_root__should_promote_successor);
    ADD_TEST(suite_rbt_remove, rbt_remove__for_stress_test_insert_remove_patterns__should_maintain_invariants);

    // RBT() - size field tracking suite
    CU_pSuite suite_rbt_size = CU_add_suite("RBT() size tracking", nullptr, nullptr);
    if (suite_rbt_size == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_rbt_size, rbt_size__for_empty_tree__should_be_zero);
    ADD_TEST(suite_rbt_size, rbt_size__after_single_insert__should_be_one);
    ADD_TEST(suite_rbt_size, rbt_size__after_multiple_inserts__should_match_node_count);
    ADD_TEST(suite_rbt_size, rbt_size__after_duplicate_insert__should_not_change);
    ADD_TEST(suite_rbt_size, rbt_size__after_single_remove__should_decrement);
    ADD_TEST(suite_rbt_size, rbt_size__after_multiple_removes__should_match_remaining_count);
    ADD_TEST(suite_rbt_size, rbt_size__after_sequential_operations__should_track_correctly);
    ADD_TEST(suite_rbt_size, rbt_size__after_stress_test__should_remain_consistent);
    ADD_TEST(suite_rbt_size, rbt_size__after_removing_nonexistent_value__should_not_change);

    // RBT() - custom comparator suite
    CU_pSuite suite_rbt_comparator = CU_add_suite("RBT() custom comparator", nullptr, nullptr);
    if (suite_rbt_comparator == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    ADD_TEST(suite_rbt_comparator, rbt__with_custom_comparator__should_use_custom_comparison);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
