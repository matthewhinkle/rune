/**
 * Red-Black Tree - Integer-valued implementation.
 *
 * Provides:
 *   - Self-balancing binary search tree with O(log n) operations
 *   - Red-Black tree invariants (color balance, structural balance)
 *   - Automatic rebalancing on insertion with color flipping and rotations
 *   - Tree visualization for debugging
 *
 * Quick Reference:
 *   rb_tree                Create empty tree with {.root = nullptr}
 *   rb_node(val)           Create new node with given value
 *   rb_insert(tree, val)   Insert value into tree (no-op if already exists)
 *   rb_print(root)         Print tree structure for debugging
 *
 * Example:
 *   rb_tree tree = {.root = nullptr};
 *   rb_insert(&tree, 50);
 *   rb_insert(&tree, 30);
 *   rb_insert(&tree, 70);
 *   rb_print(tree.root);
 *
 * Notes:
 *   - This is an int-valued implementation; future work will generify using macros
 *   - All nodes are allocated with mem_alloc_zero and tracked by the allocator
 *   - Null pointers in leaf positions are treated as black nodes
 */

#ifndef RUNE_RBT_H
#define RUNE_RBT_H

#include "r.h"

#include <stdio.h>

// ================================================================================================
// TYPE DEFINITIONS
// ================================================================================================

enum rb_color { red, black };

enum rb_dir { left, right };

struct rb_node {
    int val;
    enum rb_color color;
    struct rb_node * parent;
    struct rb_node * left;
    struct rb_node * right;
};

typedef struct {
    struct rb_node * root;
} rb_tree;

// ================================================================================================
// NODE CREATION
// ================================================================================================

static struct rb_node * rb_node(const int val) {
    struct rb_node * node = mem_alloc_zero(sizeof(struct rb_node));
    node->val = val;
    node->color = red;
    return node;
}

// ================================================================================================
// SEARCH AND NAVIGATION
// ================================================================================================

static struct rb_node * rb_parent(struct rb_node * root, const int val, struct rb_node ** out_slot) {
    struct rb_node * parent = nullptr;
    struct rb_node * cur = root;
    while (cur != nullptr && cur->val != val) {
        parent = cur;
        if (val < cur->val) {
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    if (out_slot != nullptr) {
        *out_slot = cur;
    }
    return parent;
}

static struct rb_node * rb_min(struct rb_node * node) {
    struct rb_node * cur = node;
    while (cur != nullptr && cur->left != nullptr) {
        cur = cur->left;
    }
    return cur;
}

static bool rb_isleft(const struct rb_node * node) {
    return node->parent != nullptr && node->parent->left == node;
}

// ================================================================================================
// ROTATION
// ================================================================================================

/**
 * Rotate a node left around its parent.
 *
 * Before:             After:
 *   parent              node
 *   /    \             /    \
 *  A     node    →  parent   C
 *        /  \       /    \
 *       B    C     A      B
 *
 * The node (right child) moves up to replace its parent.
 * Its left child (B) becomes the parent's new right child.
 *
 *
 * Rotate a node right around its parent.
 *
 * Before:             After:
 *    parent              node
 *    /    \             /    \
 *  node    C       →   A     parent
 *  /  \                      /    \
 * A    B                     B      C
 *
 * The node (left child) moves up to replace its parent.
 * Its right child (B) becomes the parent's new left child.
 */
static void rb_rotate(rb_tree * tree, struct rb_node * node, enum rb_dir dir) {
    struct rb_node * parent = node->parent;
    if (parent == nullptr) {
        return; // cannot rotate root
    }

    // update the parent pointers
    const struct rb_node * gparent = parent->parent;
    if (gparent == nullptr) {
        // parent is root
        tree->root = node;
    } else if (rb_isleft(parent)) {
        parent->parent->left = node;
    } else {
        parent->parent->right = node;
    }
    node->parent = parent->parent;

    // now make node's former parent its right child
    // ReSharper disable once CppDFAUnusedValue
    struct rb_node * prev = nullptr;

    if (dir == left) {
        prev = node->left;
        node->left = parent;
        parent->parent = node;
        parent->right = prev;
    } else {
        prev = node->right;
        node->right = parent;
        parent->parent = node;
        parent->left = prev;
    }

    if (prev != nullptr) {
        prev->parent = parent;
    }
}

// ================================================================================================
// INSERTION
// ================================================================================================

static void rb_insert(rb_tree * tree, const int val) {
    if (tree == nullptr) {
        return;
    }

    // 1 - find parent
    struct rb_node * slot = nullptr;
    struct rb_node * parent = rb_parent(tree->root, val, &slot);

    // 2 - check if the node exists
    if (slot != nullptr) {
        return;
    }

    // 3 - create new node
    struct rb_node * new_node = rb_node(val);

    // 4 - check if tree is empty
    if (parent == nullptr) {
        new_node->color = black;
        tree->root = new_node;
        return;
    }

    // 5 - insert new node
    new_node->parent = parent;
    if (val < parent->val) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    // now handle red parent violations
    struct rb_node * cur = new_node;

    while (cur->parent != nullptr && cur->parent->color == red) {

        // 6 - parent is red, check the uncle; leaf (null) nodes are considered black
        const bool parent_left = rb_isleft(cur->parent);
        struct rb_node * gparent = cur->parent->parent;
        struct rb_node * uncle = parent_left ? gparent->right : gparent->left;

        // 7 - check if uncle is black, if so, we need rotations and then we may exit
        if (uncle == nullptr || uncle->color == black) {
            // uncle is black
            if (rb_isleft(cur)) {
                if (parent_left) {
                    // left-left (single rotation)
                    rb_rotate(tree, cur->parent, right);
                    cur->parent->color = black;
                } else {
                    // left-right (double rotation): rotate cur right, then left
                    rb_rotate(tree, cur, right);
                    rb_rotate(tree, cur, left);
                    cur->color = black;
                }
            } else {
                if (parent_left) {
                    // right-left (double rotation): rotate cur left, then right
                    rb_rotate(tree, cur, left);
                    rb_rotate(tree, cur, right);
                    cur->color = black;
                } else {
                    // right-right (single rotation)
                    rb_rotate(tree, cur->parent, left);
                    cur->parent->color = black;
                }
            }

            gparent->color = red;
            break;
        }

        // 8 - the uncle is red, recolor and continue checking for violations up the tree
        uncle->color = black;
        cur->parent->color = black;
        gparent->color = red;
        cur = gparent;
    }

    // 9 - ensure root is black
    if (tree->root != nullptr) {
        tree->root->color = black;
    }
}

// ================================================================================================
// DELETION
// ================================================================================================

/**
 * Replace a node with its child in the tree structure.
 */
static void rb_replace(rb_tree * tree, const struct rb_node * node, struct rb_node * child) {
    if (child != nullptr) {
        child->parent = node->parent;
    }
    if (node->parent != nullptr) {
        if (rb_isleft(node)) {
            node->parent->left = child;
        } else {
            node->parent->right = child;
        }
    } else {
        tree->root = child;
    }
}

/**
 * Perform BST deletion and return the node to start RB fixup from.
 *
 * Returns the node that needs RB fixup, or nullptr if no fixup needed.
 * The returned node's color will indicate whether fixup is necessary.
 */
static struct rb_node * rb_bst_delete(rb_tree * tree, struct rb_node * node) {
    // Two children case
    if (node->left && node->right) {
        struct rb_node * successor = rb_min(node->right);

        // Extract successor from its current position
        if (successor->parent != node) {
            if (successor->right) {
                successor->right->parent = successor->parent;
            }
            successor->parent->left = successor->right;
        }

        // Replace node with successor
        successor->left = node->left;
        successor->left->parent = successor;

        if (successor->parent != node) {
            successor->right = node->right;
            successor->right->parent = successor;
        }

        successor->parent = node->parent;
        successor->color = node->color;
        rb_replace(tree, node, successor);

        mem_free(node, sizeof(struct rb_node));
        return successor;
    }

    // One child case (either left or right)
    struct rb_node * child = node->left ? node->left : node->right;
    rb_replace(tree, node, child);
    mem_free(node, sizeof(struct rb_node));
    return child;
}

static void rb_delete(rb_tree * tree, const int val) {
    if (tree == nullptr) {
        return;
    }

    struct rb_node * slot = nullptr;
    struct rb_node * parent = rb_parent(tree->root, val, &slot);

    if (slot == nullptr) {
        return; // value not found
    }

    // Perform BST deletion and get the node to start RB fixup from
    struct rb_node * fixup_node = rb_bst_delete(tree, slot);

    // TODO: RB fixup here
    // Only needed if a black node was deleted
}

// ================================================================================================
// PRINTING / VISUALIZATION
// ================================================================================================

static void rb_print_helper(const struct rb_node * node, const char * prefix, bool is_left_child) {
    if (node == nullptr) {
        return;
    }

    printf("%s%s%d (%s)\n", prefix, is_left_child ? "L-- " : "R-- ", node->val, node->color == red ? "R" : "B");

    if (node->left != nullptr || node->right != nullptr) {
        if (node->left != nullptr) {
            char new_prefix[256];
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_left_child ? "|   " : "    ");
            rb_print_helper(node->left, new_prefix, true);
        }

        if (node->right != nullptr) {
            char new_prefix[256];
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_left_child ? "|   " : "    ");
            rb_print_helper(node->right, new_prefix, false);
        }
    }
}

static void rb_print(const struct rb_node * root) {
    if (root == nullptr) {
        printf("(empty tree)\n");
        return;
    }
    printf("%d (%s)\n", root->val, root->color == red ? "R" : "B");
    if (root->left != nullptr || root->right != nullptr) {
        if (root->left != nullptr) {
            rb_print_helper(root->left, "", true);
        }
        if (root->right != nullptr) {
            rb_print_helper(root->right, "", false);
        }
    }
}

#endif // RUNE_RBT_H
