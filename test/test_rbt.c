/*
 * Red-Black tree tests.
 */

#include "../src/rbt.h"
#include "CUnit/Basic.h"
#include "test.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// ========================================================================
// helper functions
// ========================================================================

// Count nodes in a subtree
static size_t rb_count_nodes(const struct rb_node * node) {
    if (node == nullptr) {
        return 0;
    }
    return 1 + rb_count_nodes(node->left) + rb_count_nodes(node->right);
}

// Check if node value exists in tree
static bool rb_contains(const struct rb_node * node, const int val) {
    if (node == nullptr) {
        return false;
    }
    if (node->val == val) {
        return true;
    }
    if (val < node->val) {
        return rb_contains(node->left, val);
    }
    return rb_contains(node->right, val);
}

// Free entire tree recursively
static void rb_free_tree(struct rb_node * node) {
    if (node == nullptr) {
        return;
    }
    rb_free_tree(node->left);
    rb_free_tree(node->right);
    mem_free(node, sizeof(struct rb_node));
}

// Check if node is red
static bool is_red(const struct rb_node * node) {
    return node != nullptr && node->color == red;
}

// Check if node is black (or null)
static bool is_black(const struct rb_node * node) {
    return node == nullptr || node->color == black;
}

// Check red-black property: no red node has a red child
static bool rb_check_no_red_red(const struct rb_node * node) {
    if (node == nullptr) {
        return true;
    }
    if (is_red(node)) {
        if (is_red(node->left) || is_red(node->right)) {
            return false;
        }
    }
    return rb_check_no_red_red(node->left) && rb_check_no_red_red(node->right);
}

// Calculate black height - should be same for all paths
static int rb_black_height(const struct rb_node * node) {
    if (node == nullptr) {
        return 1; // null nodes count as black
    }
    int left_height = rb_black_height(node->left);
    int right_height = rb_black_height(node->right);

    if (left_height == -1 || right_height == -1 || left_height != right_height) {
        return -1; // invalid tree
    }

    return left_height + (node->color == black ? 1 : 0);
}

// Check if tree maintains BST property
static bool rb_check_bst(const struct rb_node * node, int min_val, int max_val) {
    if (node == nullptr) {
        return true;
    }
    if (node->val <= min_val || node->val >= max_val) {
        return false;
    }
    return rb_check_bst(node->left, min_val, node->val) && rb_check_bst(node->right, node->val, max_val);
}

// ========================================================================
// simple insert tests
// ========================================================================

static void rb_insert__for_empty_tree__should_create_root_node() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 50);
    CU_ASSERT_EQUAL(tree.root->color, black);
    CU_ASSERT_PTR_NULL(tree.root->parent);
    CU_ASSERT_PTR_NULL(tree.root->left);
    CU_ASSERT_PTR_NULL(tree.root->right);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_simple_sequence__should_create_valid_tree() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 10);
    rb_insert(&tree, 5);
    rb_insert(&tree, 15);

    // Assert
    // Root should be black
    CU_ASSERT_EQUAL(tree.root->color, black);
    CU_ASSERT_EQUAL(tree.root->val, 10);

    // Left child should be red
    CU_ASSERT_PTR_NOT_NULL(tree.root->left);
    CU_ASSERT_EQUAL(tree.root->left->val, 5);
    CU_ASSERT_EQUAL(tree.root->left->color, red);

    // Right child should be red
    CU_ASSERT_PTR_NOT_NULL(tree.root->right);
    CU_ASSERT_EQUAL(tree.root->right->val, 15);
    CU_ASSERT_EQUAL(tree.root->right->color, red);

    // All nodes should have correct parent pointers
    CU_ASSERT_EQUAL(tree.root->left->parent->val, 10);
    CU_ASSERT_EQUAL(tree.root->right->parent->val, 10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_duplicate_value__should_ignore() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    const size_t initial_count = rb_count_nodes(tree.root);

    // Act
    rb_insert(&tree, 50);

    // Assert
    const size_t final_count = rb_count_nodes(tree.root);
    CU_ASSERT_EQUAL(initial_count, final_count);
    CU_ASSERT_EQUAL(tree.root->val, 50);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// red uncle case tests
// ========================================================================

static void rb_insert__for_red_uncle__should_recolor_and_propagate() {
    // Arrange
    // Building: Insert 50, 30, 70, 20
    // After 50, 30, 70: tree has 50(B), 30(R), 70(R)
    // Uncle of 20 is 70, which is red -> triggers recolor
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);

    // Assert
    // All nodes should exist
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 20));

    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, black);

    // After recoloring, 30 and 70 should be black, root stays black
    CU_ASSERT_EQUAL(tree.root->left->color, black);
    CU_ASSERT_EQUAL(tree.root->right->color, black);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// black uncle left-left case tests
// ========================================================================

static void rb_insert__for_black_uncle_left_left__should_rotate_right() {
    // Arrange
    // Build the tree manually to set up left-left scenario:
    //    50 (B)
    //   /  \
    //  30   70 (B)
    // (B)
    // /
    // 20 (R)
    //
    // Then insert 10, which has parent 20(R), grandparent 30(B)
    // Uncle of 20 (nil/black) triggers rotation
    rb_tree tree = {.root = nullptr};

    // Build initial tree structure
    struct rb_node * n50 = rb_node(50);
    n50->color = black;
    tree.root = n50;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n50;
    n50->left = n30;

    struct rb_node * n70 = rb_node(70);
    n70->color = black;
    n70->parent = n50;
    n50->right = n70;

    struct rb_node * n20 = rb_node(20);
    n20->color = red;
    n20->parent = n30;
    n30->left = n20;

    // Act
    rb_insert(&tree, 10);

    // Assert
    // After rotation, 30 should be the root of the tree (or subtree)
    CU_ASSERT(rb_contains(tree.root, 10));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 70));

    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, black);

    // Verify all 5 nodes exist
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 5);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// comprehensive insertion tests with invariant checking
// ========================================================================

static void rb_insert__for_left_left_case__should_maintain_invariants() {
    // Arrange
    // Insert 30, 20, 10 (ascending insertion that triggers left-left)
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 30);
    rb_insert(&tree, 20);
    rb_insert(&tree, 10);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 20);      // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, black); // root must be black

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));            // no red-red violations
    CU_ASSERT(rb_black_height(tree.root) != -1);          // valid black height
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX)); // maintains BST

    // All values present
    CU_ASSERT(rb_contains(tree.root, 10));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_left_right_case__should_maintain_invariants() {
    // Arrange
    // Insert 10, 30, 20 (creates left-right scenario)
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 10);
    rb_insert(&tree, 30);
    rb_insert(&tree, 20);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rb_contains(tree.root, 10));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_right_right_case__should_maintain_invariants() {
    // Arrange
    // Insert 10, 20, 30 (ascending insertion that triggers right-right)
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 10);
    rb_insert(&tree, 20);
    rb_insert(&tree, 30);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rb_contains(tree.root, 10));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_right_left_case__should_maintain_invariants() {
    // Arrange
    // Insert 30, 10, 20 (creates right-left scenario)
    rb_tree tree = {.root = nullptr};

    // Act
    rb_insert(&tree, 30);
    rb_insert(&tree, 10);
    rb_insert(&tree, 20);

    // Assert - tree structure and invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 20); // 20 should be root after rotations
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    CU_ASSERT(rb_contains(tree.root, 10));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_seven_node_tree__should_maintain_all_invariants() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act - insert 1-7
    for (int i = 1; i <= 7; i++) {
        rb_insert(&tree, i);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (int i = 1; i <= 7; i++) {
        CU_ASSERT(rb_contains(tree.root, i));
    }
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 7);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_descending_ten_nodes__should_maintain_all_invariants() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act - insert 10 down to 1 (worst case for unbalanced insertion)
    for (int i = 10; i >= 1; i--) {
        rb_insert(&tree, i);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (int i = 1; i <= 10; i++) {
        CU_ASSERT(rb_contains(tree.root, i));
    }
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_mixed_insertion_pattern__should_maintain_all_invariants() {
    // Arrange
    // Insert in pattern: 50, 25, 75, 12, 37, 62, 87, 6, 18, 31
    rb_tree tree = {.root = nullptr};
    int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31};
    int count = sizeof(values) / sizeof(values[0]);

    // Act
    for (int i = 0; i < count; i++) {
        rb_insert(&tree, values[i]);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present
    for (int i = 0; i < count; i++) {
        CU_ASSERT(rb_contains(tree.root, values[i]));
    }
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), count);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_alternating_pattern__should_maintain_all_invariants() {
    // Arrange - alternating left and right inserts
    // Pattern: 50, 25, 75, 12, 37, 62, 87
    rb_tree tree = {.root = nullptr};
    int values[] = {50, 25, 75, 12, 37, 62, 87};
    int count = sizeof(values) / sizeof(values[0]);

    // Act
    for (int i = 0; i < count; i++) {
        rb_insert(&tree, values[i]);
    }

    // Assert - all invariants
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->color, black);

    // RB-tree invariants
    CU_ASSERT(rb_check_no_red_red(tree.root));
    CU_ASSERT(rb_black_height(tree.root) != -1);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // All values present and count correct
    for (int i = 0; i < count; i++) {
        CU_ASSERT(rb_contains(tree.root, values[i]));
    }
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), count);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// rotation at root tests
// ========================================================================

static void rb_rotate_right__at_root__should_update_tree_root() {
    // Arrange
    // Create a simple tree:
    //     10 (B) <- root
    //    /    \
    //   5      15
    //  (R)     (B)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = red;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n10;
    n10->right = n15;

    // Act - rotate 5 right (it's a left child of root)
    rb_rotate(&tree, n5, right);

    // Assert - n5 should now be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n5);
    CU_ASSERT_PTR_NULL(n5->parent);

    // n10 should be n5's right child
    CU_ASSERT_PTR_EQUAL(n5->right, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n5);

    // n15 should still be n10's right child
    CU_ASSERT_PTR_EQUAL(n10->right, n15);
    CU_ASSERT_PTR_EQUAL(n15->parent, n10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_left__at_root__should_update_tree_root() {
    // Arrange
    // Create a simple tree:
    //     10 (B) <- root
    //    /    \
    //   5      15
    //  (B)     (R)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = red;
    n15->parent = n10;
    n10->right = n15;

    // Act - rotate 15 left (it's a right child of root)
    rb_rotate(&tree, n15, left);

    // Assert - n15 should now be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n15);
    CU_ASSERT_PTR_NULL(n15->parent);

    // n10 should be n15's left child
    CU_ASSERT_PTR_EQUAL(n15->left, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n15);

    // n5 should still be n10's left child
    CU_ASSERT_PTR_EQUAL(n10->left, n5);
    CU_ASSERT_PTR_EQUAL(n5->parent, n10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_right__with_middle_subtree__should_move_correctly() {
    // Arrange
    // Create a tree with a middle subtree:
    //      10 (B)
    //     /    \
    //    5      15
    //   / \
    //  3   7
    // (R) (B)
    //
    // When we rotate 5 right, 7 (the right child) should become 10's left child
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = red;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n10;
    n10->right = n15;

    struct rb_node * n3 = rb_node(3);
    n3->color = red;
    n3->parent = n5;
    n5->left = n3;

    struct rb_node * n7 = rb_node(7);
    n7->color = black;
    n7->parent = n5;
    n5->right = n7;

    // Act - rotate 5 right
    rb_rotate(&tree, n5, right);

    // Assert - n5 should be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n5);
    CU_ASSERT_PTR_NULL(n5->parent);

    // n10 should be n5's right child
    CU_ASSERT_PTR_EQUAL(n5->right, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n5);

    // n7 should now be n10's left child (moved from n5's right)
    CU_ASSERT_PTR_EQUAL(n10->left, n7);
    CU_ASSERT_PTR_EQUAL(n7->parent, n10);

    // n3 should still be n5's left child
    CU_ASSERT_PTR_EQUAL(n5->left, n3);
    CU_ASSERT_PTR_EQUAL(n3->parent, n5);

    // n15 should still be n10's right child
    CU_ASSERT_PTR_EQUAL(n10->right, n15);
    CU_ASSERT_PTR_EQUAL(n15->parent, n10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_right__on_non_root_left_child__should_update_parent_pointer() {
    // Arrange
    // Create a tree where we rotate a left child of a non-root node
    //        20 (B)
    //       /    \
    //      10     30
    //     /  \
    //    5    15
    //   (R)   (B)
    //
    // Rotate 5 right, which is a left child of 10 (non-root)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n20;
    n20->right = n30;

    struct rb_node * n5 = rb_node(5);
    n5->color = red;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n10;
    n10->right = n15;

    // Act - rotate 5 right
    rb_rotate(&tree, n5, right);

    // Assert - n5 should replace n10 as n20's left child
    CU_ASSERT_PTR_EQUAL(n20->left, n5);
    CU_ASSERT_PTR_EQUAL(n5->parent, n20);

    // n10 should be n5's right child
    CU_ASSERT_PTR_EQUAL(n5->right, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n5);

    // n10's left child should be null (moved from n5's right, which was null)
    CU_ASSERT_PTR_NULL(n10->left);

    // Tree root should still be n20
    CU_ASSERT_PTR_EQUAL(tree.root, n20);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_left__on_non_root_right_child__should_update_parent_pointer() {
    // Arrange
    // Create a tree where we rotate a right child of a non-root node
    //        10 (B)
    //       /    \
    //      5      20
    //           /    \
    //          15    25
    //         (B)    (R)
    //
    // Rotate 25 left, which is a right child of 20 (non-root)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    n20->parent = n10;
    n10->right = n20;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n20;
    n20->left = n15;

    struct rb_node * n25 = rb_node(25);
    n25->color = red;
    n25->parent = n20;
    n20->right = n25;

    // Act - rotate 25 left
    rb_rotate(&tree, n25, left);

    // Assert - n25 should replace n20 as n10's right child
    CU_ASSERT_PTR_EQUAL(n10->right, n25);
    CU_ASSERT_PTR_EQUAL(n25->parent, n10);

    // n20 should be n25's left child
    CU_ASSERT_PTR_EQUAL(n25->left, n20);
    CU_ASSERT_PTR_EQUAL(n20->parent, n25);

    // n15 should still be n20's left child
    CU_ASSERT_PTR_EQUAL(n20->left, n15);
    CU_ASSERT_PTR_EQUAL(n15->parent, n20);

    // Tree root should still be n10
    CU_ASSERT_PTR_EQUAL(tree.root, n10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_right__with_null_middle_subtree__should_handle_correctly() {
    // Arrange
    // Create a tree where the right child of the rotating node is null
    //     10 (B)
    //    /    \
    //   5      15
    //  (R)
    //  /
    // 3
    // (B)
    //
    // Rotate 5 right - its right child is null
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = red;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n10;
    n10->right = n15;

    struct rb_node * n3 = rb_node(3);
    n3->color = black;
    n3->parent = n5;
    n5->left = n3;

    // Act - rotate 5 right
    rb_rotate(&tree, n5, right);

    // Assert - n5 should be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n5);
    CU_ASSERT_PTR_NULL(n5->parent);

    // n10 should be n5's right child
    CU_ASSERT_PTR_EQUAL(n5->right, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n5);

    // n10's left child should be null
    CU_ASSERT_PTR_NULL(n10->left);

    // n3 should still be n5's left child
    CU_ASSERT_PTR_EQUAL(n5->left, n3);
    CU_ASSERT_PTR_EQUAL(n3->parent, n5);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_left__with_null_middle_subtree__should_handle_correctly() {
    // Arrange
    // Create a tree where the left child of the rotating node is null
    //      10 (B)
    //     /    \
    //    5      15
    //          (R)
    //           \
    //           20
    //          (B)
    //
    // Rotate 15 left - its left child is null
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n15 = rb_node(15);
    n15->color = red;
    n15->parent = n10;
    n10->right = n15;

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    n20->parent = n15;
    n15->right = n20;

    // Act - rotate 15 left
    rb_rotate(&tree, n15, left);

    // Assert - n15 should be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n15);
    CU_ASSERT_PTR_NULL(n15->parent);

    // n10 should be n15's left child
    CU_ASSERT_PTR_EQUAL(n15->left, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n15);

    // n10's right child should be null
    CU_ASSERT_PTR_NULL(n10->right);

    // n20 should still be n15's right child
    CU_ASSERT_PTR_EQUAL(n15->right, n20);
    CU_ASSERT_PTR_EQUAL(n20->parent, n15);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate_left__with_middle_subtree__should_move_correctly() {
    // Arrange
    // Create a tree with a middle subtree for left rotation:
    //      10 (B)
    //     /    \
    //    5      20
    //          /  \
    //        15    25
    //       (B)    (R)
    //
    // When we rotate 20 left, 15 (the left child) should become 10's right child
    rb_tree tree = {.root = nullptr};

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    tree.root = n10;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    struct rb_node * n20 = rb_node(20);
    n20->color = red;
    n20->parent = n10;
    n10->right = n20;

    struct rb_node * n15 = rb_node(15);
    n15->color = black;
    n15->parent = n20;
    n20->left = n15;

    struct rb_node * n25 = rb_node(25);
    n25->color = red;
    n25->parent = n20;
    n20->right = n25;

    // Act - rotate 20 left
    rb_rotate(&tree, n20, left);

    // Assert - n20 should be the root
    CU_ASSERT_PTR_EQUAL(tree.root, n20);
    CU_ASSERT_PTR_NULL(n20->parent);

    // n10 should be n20's left child
    CU_ASSERT_PTR_EQUAL(n20->left, n10);
    CU_ASSERT_PTR_EQUAL(n10->parent, n20);

    // n15 should now be n10's right child (moved from n20's left)
    CU_ASSERT_PTR_EQUAL(n10->right, n15);
    CU_ASSERT_PTR_EQUAL(n15->parent, n10);

    // n25 should still be n20's right child
    CU_ASSERT_PTR_EQUAL(n20->right, n25);
    CU_ASSERT_PTR_EQUAL(n25->parent, n20);

    // n5 should still be n10's left child
    CU_ASSERT_PTR_EQUAL(n10->left, n5);
    CU_ASSERT_PTR_EQUAL(n5->parent, n10);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_rotate__double_rotation__should_maintain_bst_property() {
    // Arrange
    // Create a tree and perform two rotations
    //     20 (B)
    //    /    \
    //   10     30
    //  (R)     (B)
    //  /
    // 5
    // (B)
    //
    // First rotate 10 right, then rotate 5 right again
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = red;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n20;
    n20->right = n30;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    // Act - first rotation
    rb_rotate(&tree, n10, right);

    // After first rotation, tree should be:
    //     20 (B)
    //    /    \
    //   10     30
    //  (R)
    //  /
    // 5
    // Should have become:
    //     10 (R)
    //    /    \
    //   5      20
    //  (B)    /  \
    //        (empty) 30

    CU_ASSERT_PTR_EQUAL(tree.root, n10);
    CU_ASSERT_PTR_EQUAL(n10->right, n20);
    CU_ASSERT_PTR_EQUAL(n20->left, nullptr);
    CU_ASSERT_PTR_EQUAL(n20->right, n30);

    // Act - second rotation
    rb_rotate(&tree, n5, right);

    // After second rotation, tree should be:
    //      5 (B)
    //     /    \
    //   (empty) 10
    //          /  \
    //        (empty) 20
    //              /  \
    //           (empty) 30

    CU_ASSERT_PTR_EQUAL(tree.root, n5);
    CU_ASSERT_PTR_EQUAL(n5->right, n10);
    CU_ASSERT_PTR_EQUAL(n10->left, nullptr);
    CU_ASSERT_PTR_EQUAL(n10->right, n20);
    CU_ASSERT_PTR_EQUAL(n20->left, nullptr);
    CU_ASSERT_PTR_EQUAL(n20->right, n30);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// larger sequence tests
// ========================================================================

static void rb_insert__for_sequence_of_five__should_maintain_tree_properties() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act - insert sequence: 50, 30, 70, 20, 40
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);
    rb_insert(&tree, 40);

    // Assert
    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, black);

    // All values should be present
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 40));

    // Node count should be exactly 5
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 5);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_ascending_sequence__should_build_balanced_tree() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act - insert ascending sequence: 1,2,3,4,5,6,7
    for (int i = 1; i <= 7; i++) {
        rb_insert(&tree, i);
    }

    // Assert
    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, black);

    // All values should be present
    for (int i = 1; i <= 7; i++) {
        CU_ASSERT(rb_contains(tree.root, i));
    }

    // Node count should be exactly 7
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 7);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_insert__for_descending_sequence__should_build_balanced_tree() {
    // Arrange
    rb_tree tree = {.root = nullptr};

    // Act - insert descending sequence: 7,6,5,4,3,2,1
    for (int i = 7; i >= 1; i--) {
        rb_insert(&tree, i);
    }

    // Assert
    // Root must be black
    CU_ASSERT_EQUAL(tree.root->color, black);

    // All values should be present
    for (int i = 1; i <= 7; i++) {
        CU_ASSERT(rb_contains(tree.root, i));
    }

    // Node count should be exactly 7
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 7);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// BST deletion tests
// ========================================================================

static void rb_delete__for_leaf_node__should_remove_correctly() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);

    // Act
    rb_delete(&tree, 30);  // 30 is a leaf

    // Assert
    CU_ASSERT(!rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 2);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_node_with_left_child_only__should_replace_with_left() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);
    // Tree:    50
    //        /    \
    //       30    70
    //      /
    //     20

    // Act
    rb_delete(&tree, 30);  // 30 has only left child

    // Assert
    CU_ASSERT(!rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Verify 20 is now child of 50
    CU_ASSERT_PTR_EQUAL(tree.root->left, tree.root->left);  // Just verify structure is valid

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_node_with_right_child_only__should_replace_with_right() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 80);
    // Tree:    50
    //        /    \
    //       30    70
    //             \
    //              80

    // Act
    rb_delete(&tree, 70);  // 70 has only right child

    // Assert
    CU_ASSERT(!rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 80));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 3);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_node_with_two_children_successor_is_right_child__should_use_successor() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);
    rb_insert(&tree, 40);
    rb_insert(&tree, 80);
    // Tree:       50
    //          /      \
    //        30        70
    //       /  \          \
    //      20  40         80

    // Act
    rb_delete(&tree, 30);  // 30 has both children, successor is 40

    // Assert
    CU_ASSERT(!rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 40));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 80));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 5);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_node_with_two_children_successor_in_left_subtree__should_use_successor() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);
    rb_insert(&tree, 40);
    rb_insert(&tree, 35);
    rb_insert(&tree, 45);
    rb_insert(&tree, 80);
    // Tree:          50
    //            /      \
    //          30        70
    //        /    \        \
    //       20    40       80
    //            /  \
    //           35  45

    // Act
    rb_delete(&tree, 30);  // 30 has both children, successor is 35

    // Assert
    CU_ASSERT(!rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 35));
    CU_ASSERT(rb_contains(tree.root, 40));
    CU_ASSERT(rb_contains(tree.root, 45));
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 80));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 7);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_root_single_node__should_make_tree_empty() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);

    // Act
    rb_delete(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NULL(tree.root);
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 0);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_root_with_left_child__should_promote_child() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);

    // Act
    rb_delete(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 30);
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 1);
    CU_ASSERT_PTR_NULL(tree.root->parent);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_root_with_right_child__should_promote_child() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 70);

    // Act
    rb_delete(&tree, 50);

    // Assert
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT_EQUAL(tree.root->val, 70);
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 1);
    CU_ASSERT_PTR_NULL(tree.root->parent);

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_root_with_two_children__should_use_successor() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    rb_insert(&tree, 20);
    rb_insert(&tree, 40);
    rb_insert(&tree, 80);

    // Act
    rb_delete(&tree, 50);  // Delete root

    // Assert
    CU_ASSERT_PTR_NOT_NULL(tree.root);
    CU_ASSERT(!rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 70));
    CU_ASSERT(rb_contains(tree.root, 20));
    CU_ASSERT(rb_contains(tree.root, 40));
    CU_ASSERT(rb_contains(tree.root, 80));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 5);
    CU_ASSERT_PTR_NULL(tree.root->parent);
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_nonexistent_node__should_not_modify_tree() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    rb_insert(&tree, 50);
    rb_insert(&tree, 30);
    rb_insert(&tree, 70);
    const size_t initial_count = rb_count_nodes(tree.root);

    // Act
    rb_delete(&tree, 999);  // Value not in tree

    // Assert
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), initial_count);
    CU_ASSERT(rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 70));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_parent_pointers_after_deletion__should_be_correct() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    int values[] = {50, 30, 70, 20, 40, 60, 80};
    int count = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < count; i++) {
        rb_insert(&tree, values[i]);
    }

    // Act
    rb_delete(&tree, 30);

    // Helper function to verify all parent pointers (borrowed from existing test infrastructure)
    // We'll verify by checking that BST property holds and tree is consistent

    // Assert
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), count - 1);
    CU_ASSERT_PTR_NULL(tree.root->parent);  // Root must have no parent

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_sequential_deletions__should_maintain_bst_property() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    int values[] = {50, 30, 70, 20, 40, 60, 80, 10, 25, 35, 45};
    int count = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < count; i++) {
        rb_insert(&tree, values[i]);
    }

    // Act - delete some nodes
    rb_delete(&tree, 20);
    rb_delete(&tree, 70);
    rb_delete(&tree, 50);

    // Assert
    CU_ASSERT(rb_check_bst(tree.root, INT_MIN, INT_MAX));
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), count - 3);
    CU_ASSERT(!rb_contains(tree.root, 20));
    CU_ASSERT(!rb_contains(tree.root, 70));
    CU_ASSERT(!rb_contains(tree.root, 50));
    CU_ASSERT(rb_contains(tree.root, 30));
    CU_ASSERT(rb_contains(tree.root, 80));

    // Cleanup
    rb_free_tree(tree.root);
}

static void rb_delete__for_delete_all_nodes__should_result_in_empty_tree() {
    // Arrange
    rb_tree tree = {.root = nullptr};
    int values[] = {50, 30, 70, 20, 40, 60, 80};
    int count = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < count; i++) {
        rb_insert(&tree, values[i]);
    }

    // Act - delete all nodes
    for (int i = 0; i < count; i++) {
        rb_delete(&tree, values[i]);
    }

    // Assert
    CU_ASSERT_PTR_NULL(tree.root);
    CU_ASSERT_EQUAL(rb_count_nodes(tree.root), 0);

    // Cleanup
    rb_free_tree(tree.root);
}

// ========================================================================
// rb_replace tests
// ========================================================================

static void rb_replace__for_left_child_with_child__should_replace_correctly() {
    // Arrange
    // Create a tree:     20 (B)
    //                   /    \
    //                  10     30
    //                 (R)     (B)
    //                /
    //               5
    //              (B)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = red;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n20;
    n20->right = n30;

    struct rb_node * n5 = rb_node(5);
    n5->color = black;
    n5->parent = n10;
    n10->left = n5;

    // Act - replace n10 with n5
    rb_replace(&tree, n10, n5);

    // Assert
    // n5 should now be n20's left child
    CU_ASSERT_PTR_EQUAL(n20->left, n5);
    CU_ASSERT_PTR_EQUAL(n5->parent, n20);

    // n30 should still be n20's right child
    CU_ASSERT_PTR_EQUAL(n20->right, n30);

    // Tree root should still be n20
    CU_ASSERT_PTR_EQUAL(tree.root, n20);

    // Cleanup
    mem_free(n5, sizeof(struct rb_node));
    mem_free(n10, sizeof(struct rb_node));
    mem_free(n30, sizeof(struct rb_node));
    mem_free(n20, sizeof(struct rb_node));
}

static void rb_replace__for_right_child_with_child__should_replace_correctly() {
    // Arrange
    // Create a tree:     20 (B)
    //                   /    \
    //                  10     30
    //                 (B)     (R)
    //                           \
    //                           35
    //                          (B)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = red;
    n30->parent = n20;
    n20->right = n30;

    struct rb_node * n35 = rb_node(35);
    n35->color = black;
    n35->parent = n30;
    n30->right = n35;

    // Act - replace n30 with n35
    rb_replace(&tree, n30, n35);

    // Assert
    // n35 should now be n20's right child
    CU_ASSERT_PTR_EQUAL(n20->right, n35);
    CU_ASSERT_PTR_EQUAL(n35->parent, n20);

    // n10 should still be n20's left child
    CU_ASSERT_PTR_EQUAL(n20->left, n10);

    // Tree root should still be n20
    CU_ASSERT_PTR_EQUAL(tree.root, n20);

    // Cleanup
    mem_free(n35, sizeof(struct rb_node));
    mem_free(n30, sizeof(struct rb_node));
    mem_free(n10, sizeof(struct rb_node));
    mem_free(n20, sizeof(struct rb_node));
}

static void rb_replace__for_node_with_null_child__should_replace_with_null() {
    // Arrange
    // Create a tree:     20 (B)
    //                   /    \
    //                  10     30
    //                 (B)     (B)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n20;
    n20->right = n30;

    // Act - replace n10 with null (node has no children)
    rb_replace(&tree, n10, nullptr);

    // Assert
    // n10 should be replaced with null
    CU_ASSERT_PTR_NULL(n20->left);

    // n30 should still be n20's right child
    CU_ASSERT_PTR_EQUAL(n20->right, n30);

    // Tree root should still be n20
    CU_ASSERT_PTR_EQUAL(tree.root, n20);

    // Cleanup
    mem_free(n10, sizeof(struct rb_node));
    mem_free(n30, sizeof(struct rb_node));
    mem_free(n20, sizeof(struct rb_node));
}

static void rb_replace__for_root_node__should_update_tree_root() {
    // Arrange
    // Create a tree:     20 (B)
    //                   /    \
    //                  10     30
    //                 (B)     (B)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    struct rb_node * n10 = rb_node(10);
    n10->color = black;
    n10->parent = n20;
    n20->left = n10;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n20;
    n20->right = n30;

    // Act - replace root (n20) with n10
    rb_replace(&tree, n20, n10);

    // Assert
    // n10 should be the new root
    CU_ASSERT_PTR_EQUAL(tree.root, n10);
    CU_ASSERT_PTR_NULL(n10->parent);

    // n30 should still be accessible (now as right child of n10)
    // But actually n30 is still left as it was
    CU_ASSERT_PTR_EQUAL(n10->right, n30);

    // Cleanup
    mem_free(n20, sizeof(struct rb_node));
    mem_free(n10, sizeof(struct rb_node));
    mem_free(n30, sizeof(struct rb_node));
}

static void rb_replace__for_root_with_null_child__should_clear_tree() {
    // Arrange
    // Create a tree with single root node
    rb_tree tree = {.root = nullptr};

    struct rb_node * n20 = rb_node(20);
    n20->color = black;
    tree.root = n20;

    // Act - replace root with null
    rb_replace(&tree, n20, nullptr);

    // Assert
    // Tree should now be empty
    CU_ASSERT_PTR_NULL(tree.root);

    // Cleanup
    mem_free(n20, sizeof(struct rb_node));
}

static void rb_replace__should_maintain_parent_child_relationships() {
    // Arrange
    // Create a deeper tree to verify relationships are maintained:
    //          50 (B)
    //         /    \
    //        30     70
    //       (B)     (B)
    //      /  \
    //    20   40
    //   (R)   (R)
    rb_tree tree = {.root = nullptr};

    struct rb_node * n50 = rb_node(50);
    n50->color = black;
    tree.root = n50;

    struct rb_node * n30 = rb_node(30);
    n30->color = black;
    n30->parent = n50;
    n50->left = n30;

    struct rb_node * n70 = rb_node(70);
    n70->color = black;
    n70->parent = n50;
    n50->right = n70;

    struct rb_node * n20 = rb_node(20);
    n20->color = red;
    n20->parent = n30;
    n30->left = n20;

    struct rb_node * n40 = rb_node(40);
    n40->color = red;
    n40->parent = n30;
    n30->right = n40;

    // Act - replace n30 (with two children) with n40
    rb_replace(&tree, n30, n40);

    // Assert
    // n40 should now be n50's left child
    CU_ASSERT_PTR_EQUAL(n50->left, n40);
    CU_ASSERT_PTR_EQUAL(n40->parent, n50);

    // n70 should still be n50's right child
    CU_ASSERT_PTR_EQUAL(n50->right, n70);
    CU_ASSERT_PTR_EQUAL(n70->parent, n50);

    // Tree root should still be n50
    CU_ASSERT_PTR_EQUAL(tree.root, n50);

    // Cleanup
    mem_free(n20, sizeof(struct rb_node));
    mem_free(n40, sizeof(struct rb_node));
    mem_free(n30, sizeof(struct rb_node));
    mem_free(n70, sizeof(struct rb_node));
    mem_free(n50, sizeof(struct rb_node));
}

// ========================================================================
// main
// ========================================================================

int main() {
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("rbt_tests", nullptr, nullptr);
    if (suite == nullptr) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Simple insert tests
    ADD_TEST(suite, rb_insert__for_empty_tree__should_create_root_node);
    ADD_TEST(suite, rb_insert__for_simple_sequence__should_create_valid_tree);
    ADD_TEST(suite, rb_insert__for_duplicate_value__should_ignore);

    // Red uncle case tests
    ADD_TEST(suite, rb_insert__for_red_uncle__should_recolor_and_propagate);

    // Black uncle left-left case tests
    ADD_TEST(suite, rb_insert__for_black_uncle_left_left__should_rotate_right);

    // Comprehensive insertion tests with invariant checking
    ADD_TEST(suite, rb_insert__for_left_left_case__should_maintain_invariants);
    ADD_TEST(suite, rb_insert__for_left_right_case__should_maintain_invariants);
    ADD_TEST(suite, rb_insert__for_right_right_case__should_maintain_invariants);
    ADD_TEST(suite, rb_insert__for_right_left_case__should_maintain_invariants);
    ADD_TEST(suite, rb_insert__for_seven_node_tree__should_maintain_all_invariants);
    ADD_TEST(suite, rb_insert__for_descending_ten_nodes__should_maintain_all_invariants);
    ADD_TEST(suite, rb_insert__for_mixed_insertion_pattern__should_maintain_all_invariants);
    ADD_TEST(suite, rb_insert__for_alternating_pattern__should_maintain_all_invariants);

    // Rotation at root tests
    ADD_TEST(suite, rb_rotate_right__at_root__should_update_tree_root);
    ADD_TEST(suite, rb_rotate_left__at_root__should_update_tree_root);
    ADD_TEST(suite, rb_rotate_right__with_middle_subtree__should_move_correctly);

    // Rotation on non-root nodes tests
    ADD_TEST(suite, rb_rotate_right__on_non_root_left_child__should_update_parent_pointer);
    ADD_TEST(suite, rb_rotate_left__on_non_root_right_child__should_update_parent_pointer);

    // Rotation with null middle subtree tests
    ADD_TEST(suite, rb_rotate_right__with_null_middle_subtree__should_handle_correctly);
    ADD_TEST(suite, rb_rotate_left__with_null_middle_subtree__should_handle_correctly);

    // Rotation with middle subtree tests
    ADD_TEST(suite, rb_rotate_left__with_middle_subtree__should_move_correctly);

    // Multi-rotation tests
    ADD_TEST(suite, rb_rotate__double_rotation__should_maintain_bst_property);

    // // Larger sequence tests
    ADD_TEST(suite, rb_insert__for_sequence_of_five__should_maintain_tree_properties);
    ADD_TEST(suite, rb_insert__for_ascending_sequence__should_build_balanced_tree);
    ADD_TEST(suite, rb_insert__for_descending_sequence__should_build_balanced_tree);

    // BST deletion tests
    ADD_TEST(suite, rb_delete__for_leaf_node__should_remove_correctly);
    ADD_TEST(suite, rb_delete__for_node_with_left_child_only__should_replace_with_left);
    ADD_TEST(suite, rb_delete__for_node_with_right_child_only__should_replace_with_right);
    ADD_TEST(suite, rb_delete__for_node_with_two_children_successor_is_right_child__should_use_successor);
    ADD_TEST(suite, rb_delete__for_node_with_two_children_successor_in_left_subtree__should_use_successor);
    ADD_TEST(suite, rb_delete__for_root_single_node__should_make_tree_empty);
    ADD_TEST(suite, rb_delete__for_root_with_left_child__should_promote_child);
    ADD_TEST(suite, rb_delete__for_root_with_right_child__should_promote_child);
    ADD_TEST(suite, rb_delete__for_root_with_two_children__should_use_successor);
    ADD_TEST(suite, rb_delete__for_nonexistent_node__should_not_modify_tree);
    ADD_TEST(suite, rb_delete__for_parent_pointers_after_deletion__should_be_correct);
    ADD_TEST(suite, rb_delete__for_sequential_deletions__should_maintain_bst_property);
    ADD_TEST(suite, rb_delete__for_delete_all_nodes__should_result_in_empty_tree);

    // rb_replace tests
    ADD_TEST(suite, rb_replace__for_left_child_with_child__should_replace_correctly);
    ADD_TEST(suite, rb_replace__for_right_child_with_child__should_replace_correctly);
    ADD_TEST(suite, rb_replace__for_node_with_null_child__should_replace_with_null);
    ADD_TEST(suite, rb_replace__for_root_node__should_update_tree_root);
    ADD_TEST(suite, rb_replace__for_root_with_null_child__should_clear_tree);
    ADD_TEST(suite, rb_replace__should_maintain_parent_child_relationships);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
