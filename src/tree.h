/**
 * Red-black self-balancing binary search tree.
 *
 * Provides:
 *   - Self-balancing binary search tree with O(log n) operations
 *   - Generic type support via macro-based template expansion
 *   - Custom comparator support with default numeric comparison
 *   - Efficient insertion, search, and deletion operations
 *   - Proper handling of red-black tree invariants
 *
 * Quick Reference:
 *
 *   Red-Black Tree API
 *   -------------------------------------------------------------------------------------------------------------------
 *   rbt(type, ...)           Create empty tree (cmp_fn optional, defaults to numeric comparison)
 *   rbt_contains(t, val)     Check if value exists
 *   rbt_insert(t, val)       Insert value (ignored if duplicate)
 *   rbt_remove(t, val)       Remove value from tree
 *
 *   BST Helper API
 *   -------------------------------------------------------------------------------------------------------------------
 *   bst_min(node)            Find minimum node in subtree
 *   bst_find(t, val)         Find node with value in tree
 *   bst_remove(t, node)      Remove specific node from tree (internal use)
 *
 * Example:
 *   // Basic tree usage
 *   #define T int
 *   #include "tree.h"
 *   RBT(int) tree = rbt(int);  // Uses default numeric comparison
 *   rbt_insert(&tree, 5);
 *   rbt_insert(&tree, 3);
 *   rbt_insert(&tree, 7);
 *   bool found = rbt_contains(&tree, 3);  // true
 *   rbt_remove(&tree, 3);
 *   #undef T
 *
 *   // With custom comparator
 *   int cmp(int a, int b) { return (a > b) - (a < b); }
 *   RBT(int) tree2 = rbt(int, cmp);
 *   rbt_insert(&tree2, 5);
 *
 * Note: RBT requires template expansion via #define T / #undef T for type instantiation.
 */

// ReSharper disable once CppMissingIncludeGuard

#include "r.h"

// =====================================================================================================================
// Binary Search Tree
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

#ifndef RUNE_BST_API
#define RUNE_BST_API

#define R_BST_CMP(t, a, b) ((t)->cmp_fn == nullptr ? ((a) > (b)) - ((a) < (b)) : (t)->cmp_fn((a), (b)))

#define R_BST_ISLEFT(node) (node)->parent != nullptr && (node)->parent->left == (node)

#define bst_min(node)                                                                                                  \
    ({                                                                                                                 \
        auto cur = (node);                                                                                             \
        while (cur != nullptr && cur->left != nullptr) {                                                               \
            cur = cur->left;                                                                                           \
        }                                                                                                              \
        /* return */ cur;                                                                                              \
    })

#define bst_find(t, val)                                                                                               \
    ({                                                                                                                 \
        auto search_cur = (t)->root;                                                                                   \
        while (search_cur != nullptr && R_BST_CMP((t), (val), search_cur->data) != 0) {                                \
            search_cur = R_BST_CMP((t), (val), search_cur->data) < 0 ? search_cur->left : search_cur->right;           \
        }                                                                                                              \
        /* return */ search_cur;                                                                                       \
    })

/**
 * Replace a node with its child in the tree (used in BST deletion).
 */
#define R_BST_REPLACE(t, node, child)                                                                                  \
    ({                                                                                                                 \
        if ((child) != nullptr) {                                                                                      \
            (child)->parent = (node)->parent;                                                                          \
        }                                                                                                              \
        if ((node)->parent != nullptr) {                                                                               \
            if (R_BST_ISLEFT((node))) {                                                                                \
                (node)->parent->left = (child);                                                                        \
            } else {                                                                                                   \
                (node)->parent->right = (child);                                                                       \
            }                                                                                                          \
        } else {                                                                                                       \
            (t)->root = (child);                                                                                       \
        }                                                                                                              \
    })

/**
 * Replace a node with its successor (used in two-child removal).
 * Handles disconnecting the successor from its current position and attaching
 * it to the node's children.
 */
#define R_BST_SUCCEED(t, node, successor)                                                                              \
    ({                                                                                                                 \
        /* If successor is not the direct right child, disconnect it from old position */                              \
        if ((successor)->parent != (node)) {                                                                           \
            typeof_unqual((successor)) successor_right = (successor)->right;                                           \
            typeof_unqual((successor)) successor_parent = (successor)->parent;                                         \
            /* Replace successor with its right child in its old parent's tree */                                      \
            if (successor_parent->left == (successor)) {                                                               \
                successor_parent->left = successor_right;                                                              \
            } else {                                                                                                   \
                successor_parent->right = successor_right;                                                             \
            }                                                                                                          \
            /* Update right child's parent if it exists */                                                             \
            if (successor_right != nullptr) {                                                                          \
                successor_right->parent = successor_parent;                                                            \
            }                                                                                                          \
            /* Attach node's right subtree to successor */                                                             \
            (successor)->right = (node)->right;                                                                        \
            if ((node)->right != nullptr) {                                                                            \
                (node)->right->parent = (successor);                                                                   \
            }                                                                                                          \
        }                                                                                                              \
                                                                                                                       \
        /* Attach node's left subtree to successor */                                                                  \
        (successor)->left = (node)->left;                                                                              \
        if ((node)->left != nullptr) {                                                                                 \
            (node)->left->parent = (successor);                                                                        \
        }                                                                                                              \
                                                                                                                       \
        /* Copy RBT color */                                                                                           \
        (successor)->color = (node)->color;                                                                            \
        /* Replace node with successor in the tree */                                                                  \
        (successor)->parent = (node)->parent;                                                                          \
        if ((node)->parent != nullptr) {                                                                               \
            if (R_BST_ISLEFT((node))) {                                                                                \
                (node)->parent->left = (successor);                                                                    \
            } else {                                                                                                   \
                (node)->parent->right = (successor);                                                                   \
            }                                                                                                          \
        } else {                                                                                                       \
            (t)->root = (successor);                                                                                   \
        }                                                                                                              \
    })

#define bst_remove(t, node)                                                                                            \
    ({                                                                                                                 \
        typeof_unqual(node) suc = nullptr;                                                                             \
        typeof_unqual(node) node_to_free = (node);                                                                     \
        if ((node)->left == nullptr) {                                                                                 \
            suc = (node)->right;                                                                                       \
            R_BST_REPLACE((t), (node), (node)->right);                                                                 \
        } else if ((node)->right == nullptr) {                                                                         \
            suc = (node)->left;                                                                                        \
            R_BST_REPLACE((t), (node), (node)->left);                                                                  \
        } else {                                                                                                       \
            /* node has left and right children */                                                                     \
            auto min_right = bst_min((node)->right);                                                                   \
            R_BST_SUCCEED((t), (node), min_right);                                                                     \
            suc = min_right;                                                                                           \
        }                                                                                                              \
        /* Clear node's pointers before freeing to prevent dangling references */                                      \
        node_to_free->left = nullptr;                                                                                  \
        node_to_free->right = nullptr;                                                                                 \
        node_to_free->parent = nullptr;                                                                                \
        (t)->node_free_fn(node_to_free);                                                                               \
        /* return */ suc;                                                                                              \
    })

#endif // RUNE_BST_API

// =====================================================================================================================
// Red-Black Tree
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

#ifndef RUNE_RBT_API
#define RUNE_RBT_API

enum rbt_color { R_(rbt_red), R_(rbt_black), R_(rbt_double_black) };

enum rbt_dir { R_(rbt_left), R_(rbt_right), R_(rbt_exists) };

#define RBT(type) R_GLUE(rbt_, type)
#define RBT_NODE(type) R_GLUE(rbt_node_, type)
#define R_RBT_FREE(type) R_GLUE(RBT(type), _free)
#define R_RBT_NODE_NEW(type) R_GLUE(RBT_NODE(type), _new)
#define R_RBT_NODE_FREE(type) R_GLUE(RBT_NODE(type), _free)

#define rbt(type, ...)                                                                                                 \
    {.root = nullptr,                                                                                                  \
     .node_fn = R_RBT_NODE_NEW(type),                                                                                  \
     .node_free_fn = R_RBT_NODE_FREE(type) __VA_OPT__(, .cmp_fn = __VA_ARGS__)}

#define R_RBT_PARENT(t, val, out_dir)                                                                                  \
    ({                                                                                                                 \
        typeof_unqual((t)->root) search_parent = nullptr;                                                              \
        auto search_cur = (t)->root;                                                                                   \
        int cmp_result = 0;                                                                                            \
        while (search_cur != nullptr && (cmp_result = R_BST_CMP((t), (val), search_cur->data)) != 0) {                 \
            search_parent = search_cur;                                                                                \
            search_cur = cmp_result < 0 ? search_cur->left : search_cur->right;                                        \
        }                                                                                                              \
        if ((out_dir) != nullptr) {                                                                                    \
            if (search_cur == nullptr) {                                                                               \
                *(out_dir) = cmp_result < 0 ? R_(rbt_left) : R_(rbt_right);                                            \
            } else {                                                                                                   \
                *(out_dir) = R_(rbt_exists);                                                                           \
            }                                                                                                          \
        }                                                                                                              \
                                                                                                                       \
        /* return */ search_parent;                                                                                    \
    })

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
#define R_RBT_ROTATE(t, node, dir)                                                                                     \
    ({                                                                                                                 \
        auto node_parent = (node)->parent;                                                                             \
        if (node_parent != nullptr) {                                                                                  \
            /* cannot rotate root */                                                                                   \
                                                                                                                       \
            /* update the parent pointers */                                                                           \
            const auto rotate_gparent = node_parent->parent;                                                           \
            if (rotate_gparent == nullptr) {                                                                           \
                /* parent is root */                                                                                   \
                t->root = (node);                                                                                      \
            } else if (R_BST_ISLEFT(node_parent)) {                                                                    \
                rotate_gparent->left = (node);                                                                         \
            } else {                                                                                                   \
                rotate_gparent->right = (node);                                                                        \
            }                                                                                                          \
            (node)->parent = node_parent->parent;                                                                      \
                                                                                                                       \
            /* node make node's former parent its right child */                                                       \
            typeof_unqual((t)->root) prev = nullptr;                                                                   \
            if (dir == R_(rbt_left)) {                                                                                 \
                prev = (node)->left;                                                                                   \
                (node)->left = node_parent;                                                                            \
                node_parent->parent = (node);                                                                          \
                node_parent->right = prev;                                                                             \
            } else {                                                                                                   \
                prev = node->right;                                                                                    \
                node->right = node_parent;                                                                             \
                node_parent->parent = node;                                                                            \
                node_parent->left = prev;                                                                              \
            }                                                                                                          \
                                                                                                                       \
            if (prev != nullptr) {                                                                                     \
                prev->parent = node_parent;                                                                            \
            }                                                                                                          \
        }                                                                                                              \
    })

/**
 * Get the color of a node's sibling.
 * Assumes parent is not null. Treats null siblings as black.
 */
#define R_RBT_SIB_COLOR(node)                                                                                          \
    (R_BST_ISLEFT((node)) ? ((node)->parent->right != nullptr ? (node)->parent->right->color : R_(rbt_black))          \
                          : ((node)->parent->left != nullptr ? (node)->parent->left->color : R_(rbt_black)))

#define rbt_contains(t, val) (((t) != nullptr) && (bst_find((t), (val)) != nullptr))

#define rbt_insert(t, val)                                                                                             \
    ({                                                                                                                 \
        if ((t) != nullptr) {                                                                                          \
            /* 1 - find the parent and direction */                                                                    \
            enum rbt_dir dir = R_(rbt_exists);                                                                         \
            auto parent = R_RBT_PARENT((t), (val), &dir);                                                              \
                                                                                                                       \
            /* 2 - check if the node already exists */                                                                 \
            if (dir != R_(rbt_exists)) {                                                                               \
                /* 3 - create the new node */                                                                          \
                auto new_node = (t)->node_fn(val);                                                                     \
                new_node->color = R_(rbt_red);                                                                         \
                new_node->data = (val);                                                                                \
                                                                                                                       \
                /* 4 - check if the tree is empty */                                                                   \
                if (parent == nullptr) {                                                                               \
                    new_node->color = R_(rbt_black);                                                                   \
                    (t)->root = new_node;                                                                              \
                } else {                                                                                               \
                    /* 5 - insert the new node using the direction from R_RBT_PARENT */                                \
                    new_node->parent = parent;                                                                         \
                    if (dir == R_(rbt_left)) {                                                                         \
                        parent->left = new_node;                                                                       \
                    } else {                                                                                           \
                        parent->right = new_node;                                                                      \
                    }                                                                                                  \
                                                                                                                       \
                    /* 6 - handle red parent violations */                                                             \
                    auto cur = new_node;                                                                               \
                                                                                                                       \
                    while (cur->parent != nullptr && cur->parent->color == R_(rbt_red)) {                              \
                        /* 7 - parent is red, check the uncle; leaf (null) nodes are considered black */               \
                        const bool parent_left = R_BST_ISLEFT(cur->parent);                                            \
                        auto gparent = cur->parent->parent;                                                            \
                        auto uncle = parent_left ? gparent->right : gparent->left;                                     \
                                                                                                                       \
                        /* 8 - check if uncle is black, if so, we need rotations and then exit */                      \
                        if (uncle == nullptr || uncle->color == R_(rbt_black)) {                                       \
                            /* uncle is black */                                                                       \
                            if (R_BST_ISLEFT(cur)) {                                                                   \
                                if (parent_left) {                                                                     \
                                    /* left-left (single rotation) */                                                  \
                                    R_RBT_ROTATE((t), cur->parent, R_(rbt_right));                                     \
                                    cur->parent->color = R_(rbt_black);                                                \
                                } else {                                                                               \
                                    /* left-right (double rotation): rotate cur right, then left */                    \
                                    R_RBT_ROTATE((t), cur, R_(rbt_right));                                             \
                                    R_RBT_ROTATE((t), cur, R_(rbt_left));                                              \
                                    cur->color = R_(rbt_black);                                                        \
                                }                                                                                      \
                            } else {                                                                                   \
                                if (parent_left) {                                                                     \
                                    /* right-left (double rotation): rotate cur left, then right */                    \
                                    R_RBT_ROTATE((t), cur, R_(rbt_left));                                              \
                                    R_RBT_ROTATE((t), cur, R_(rbt_right));                                             \
                                    cur->color = R_(rbt_black);                                                        \
                                } else {                                                                               \
                                    /* right-right (single rotation) */                                                \
                                    R_RBT_ROTATE((t), cur->parent, R_(rbt_left));                                      \
                                    cur->parent->color = R_(rbt_black);                                                \
                                }                                                                                      \
                            }                                                                                          \
                                                                                                                       \
                            gparent->color = R_(rbt_red);                                                              \
                            break;                                                                                     \
                        }                                                                                              \
                                                                                                                       \
                        /* 9 - the uncle is red, recolor and continue checking for violations up tree */               \
                        uncle->color = R_(rbt_black);                                                                  \
                        cur->parent->color = R_(rbt_black);                                                            \
                        gparent->color = R_(rbt_red);                                                                  \
                        cur = gparent;                                                                                 \
                    }                                                                                                  \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        /* 10 - ensure root is black */                                                                                \
        if ((t) != nullptr && (t)->root != nullptr) {                                                                  \
            (t)->root->color = R_(rbt_black);                                                                          \
        }                                                                                                              \
    })

/**
 * Fix red-black tree violations after deleting a black node.
 * Handles the double-black node by applying one of four cases:
 *
 * CASE 1: Sibling is RED
 *   The parent must be black (RBT property). Rotate to convert this into
 *   one of the other cases where the sibling is black.
 *
 *   Before:              After:              After Recolor:
 *     P(B)                S(B)                  S(B)
 *    /  \                /  \                 /  \
 *   DB  S(R)    →    P(R) SR(B)    →    P(R)  SR(B)
 *       /  \          /  \                /  \
 *      SL  SR       DB   SL             DB   SL
 *
 *   Action: Rotate parent around sibling, recolor sibling to black and parent to red.
 *
 * CASE 2: Sibling is BLACK with both children BLACK
 *   This is the "simple" case. Recolor sibling to red and move the double-black
 *   violation up to the parent. This reduces black-height of the sibling's
 *   subtree, balancing it with the double-black side temporarily.
 *
 *   Before:              After:
 *     P                    P(DB)
 *    / \                  / \
 *   DB  S(B)    →       DB  S(R)
 *       / \              / \
 *      SL  SR          SL   SR
 *
 *   Action: Recolor sibling to red, move double-black up to parent, continue fixing.
 *
 * CASE 3: Sibling is BLACK, near child is RED (far child is BLACK)
 *   This is a setup case. Rotate the sibling to move the red child to the "far"
 *   position (away from the double-black), which converts this to Case 4.
 *   Shown for DB as left child (mirror for right):
 *
 *   Before:              After:
 *     P                    P
 *    / \                  / \
 *   DB  S(B)    →       DB  SL(B)
 *       / \                 / \
 *     SL(R) SR         SL_L  S(R)
 *                             \
 *                             SR(B)
 *
 *   Action: Rotate sibling, recolor to set up for Case 4. Loop continues.
 *
 * CASE 4: Sibling is BLACK with far child RED (the final fix)
 *   This case fully resolves the violation in one operation. Rotate the parent
 *   around the sibling, and recolor such that black-height is restored.
 *   Shown for DB as left child (mirror for right):
 *
 *   Before:              After:
 *     P                    S(P's color)
 *    / \                  / \
 *   DB  S(B)    →      P(B)  SR(B)
 *       / \             / \
 *      SL  SR(R)     DB  SL
 *
 *   Action: Rotate parent, recolor, double-black is eliminated.
 *
 * The loop continues until the double-black reaches the root or is eliminated.
 * If it reaches the root, we simply make it single black (no violation at root).
 */
#define R_RBT_FIX_DELETE(t, node)                                                                                      \
    ({                                                                                                                 \
        while ((node) != nullptr && (node)->color == R_(rbt_double_black) && (node) != (t)->root) {                    \
            const bool is_left = R_BST_ISLEFT((node));                                                                 \
            auto sibling = is_left ? (node)->parent->right : (node)->parent->left;                                     \
                                                                                                                       \
            if (sibling != nullptr && sibling->color == R_(rbt_red)) {                                                 \
                /* CASE 1: Sibling is RED - rotate and recolor to convert to cases 2-4 */                              \
                const enum rbt_color parent_color = (node)->parent->color;                                             \
                if (is_left) {                                                                                         \
                    R_RBT_ROTATE((t), sibling, R_(rbt_left));                                                          \
                } else {                                                                                               \
                    R_RBT_ROTATE((t), sibling, R_(rbt_right));                                                         \
                }                                                                                                      \
                sibling->color = parent_color;                                                                         \
                (node)->parent->color = R_(rbt_red);                                                                   \
                /* Get new sibling after rotation (was child of old sibling) */                                        \
                sibling = is_left ? (node)->parent->right : (node)->parent->left;                                      \
            }                                                                                                          \
                                                                                                                       \
            if (sibling != nullptr) {                                                                                  \
                const bool sl_red = sibling->left != nullptr && sibling->left->color == R_(rbt_red);                   \
                const bool sr_red = sibling->right != nullptr && sibling->right->color == R_(rbt_red);                 \
                                                                                                                       \
                if (!sl_red && !sr_red) {                                                                              \
                    /* CASE 2: Sibling BLACK, both children BLACK */                                                   \
                    sibling->color = R_(rbt_red);                                                                      \
                    (node)->color = R_(rbt_black);                                                                     \
                    if ((node)->parent != nullptr && (node)->parent->color == R_(rbt_red)) {                           \
                        /* Parent is RED - resolve immediately */                                                      \
                        (node)->parent->color = R_(rbt_black);                                                         \
                        (node) = nullptr; /* Exit loop - violation resolved */                                         \
                    } else {                                                                                           \
                        /* Parent is BLACK - move violation up */                                                      \
                        (node) = (node)->parent;                                                                       \
                        if ((node) != nullptr) {                                                                       \
                            (node)->color = R_(rbt_double_black);                                                      \
                        }                                                                                              \
                    }                                                                                                  \
                } else if ((is_left && sr_red) || (!is_left && sl_red)) {                                              \
                    /* CASE 4: Far child RED - final fix eliminates double-black */                                    \
                    if (is_left) {                                                                                     \
                        const enum rbt_color parent_color = (node)->parent->color;                                     \
                        R_RBT_ROTATE((t), sibling, R_(rbt_left));                                                      \
                        sibling->color = parent_color;                                                                 \
                        sibling->left->color = R_(rbt_black);                                                          \
                        sibling->right->color = R_(rbt_black);                                                         \
                    } else {                                                                                           \
                        const enum rbt_color parent_color = (node)->parent->color;                                     \
                        R_RBT_ROTATE((t), sibling, R_(rbt_right));                                                     \
                        sibling->color = parent_color;                                                                 \
                        sibling->left->color = R_(rbt_black);                                                          \
                        sibling->right->color = R_(rbt_black);                                                         \
                    }                                                                                                  \
                    (node)->color = R_(rbt_black);                                                                     \
                    (node) = nullptr; /* Exit loop - violation resolved */                                             \
                } else {                                                                                               \
                    /* CASE 3: Near child RED, far child BLACK - setup for case 4 */                                   \
                    auto near_child = is_left ? sibling->left : sibling->right;                                        \
                    if (is_left) {                                                                                     \
                        R_RBT_ROTATE((t), near_child, R_(rbt_right));                                                  \
                    } else {                                                                                           \
                        R_RBT_ROTATE((t), near_child, R_(rbt_left));                                                   \
                    }                                                                                                  \
                    /* After rotation, get new sibling */                                                              \
                    sibling = is_left ? (node)->parent->right : (node)->parent->left;                                  \
                    sibling->color = R_(rbt_black);                                                                    \
                    /* Old sibling is now a child of new sibling - color it red */                                     \
                    auto old_sibling = is_left ? sibling->right : sibling->left;                                       \
                    old_sibling->color = R_(rbt_red);                                                                  \
                    /* Loop continues, now in Case 4 */                                                                \
                }                                                                                                      \
            } else {                                                                                                   \
                /* No sibling - make double-black single black */                                                      \
                (node)->color = R_(rbt_black);                                                                         \
            }                                                                                                          \
        }                                                                                                              \
        /* If root is double-black, make it single black */                                                            \
        if ((node) != nullptr && (node)->color == R_(rbt_double_black)) {                                              \
            (node)->color = R_(rbt_black);                                                                             \
        }                                                                                                              \
    })

#define rbt_remove(t, val)                                                                                             \
    ({                                                                                                                 \
        typeof_unqual((t)->root) node = nullptr;                                                                       \
        if ((t) != nullptr) {                                                                                          \
            node = bst_find((t), (val));                                                                               \
            if (node != nullptr) {                                                                                     \
                const enum rbt_color node_color = node->color;                                                         \
                typeof_unqual((t)->root) parent = node->parent;                                                        \
                const bool is_left_child = parent != nullptr && R_BST_ISLEFT(node);                                    \
                /* For two-children case, save successor's right child (will replace successor) */                     \
                typeof_unqual((t)->root) suc_right_child = nullptr;                                                    \
                typeof_unqual((t)->root) suc_parent_saved = nullptr;                                                   \
                bool suc_was_left_child = false;                                                                       \
                if (node->left != nullptr && node->right != nullptr) {                                                 \
                    auto successor = bst_min(node->right);                                                             \
                    suc_right_child = successor->right;                                                                \
                    suc_parent_saved = successor->parent;                                                              \
                    suc_was_left_child = successor->parent != nullptr && R_BST_ISLEFT(successor);                      \
                }                                                                                                      \
                const enum rbt_color suc_color =                                                                       \
                    (node->left == nullptr && node->right == nullptr)                                                  \
                        ? R_(rbt_black)                                                                                \
                        : (node->left == nullptr                                                                       \
                               ? node->right->color                                                                    \
                               : (node->right == nullptr ? node->left->color : bst_min(node->right)->color));          \
                typeof_unqual((t)->root) node_suc = bst_remove((t), (node));                                           \
                /* Only fix if both deleted node and replacement were black */                                         \
                if (node_color == R_(rbt_black) && suc_color == R_(rbt_black) && node_suc != nullptr) {                \
                    if (suc_right_child != nullptr || suc_parent_saved != nullptr) {                                   \
                        /* Two-children case: fix at successor's old position */                                       \
                        const enum rbt_color rc_color =                                                                \
                            suc_right_child != nullptr ? suc_right_child->color : R_(rbt_black);                       \
                        if (rc_color == R_(rbt_red)) {                                                                 \
                            /* Right child was red - just color it black */                                            \
                            suc_right_child->color = R_(rbt_black);                                                    \
                        } else if (suc_right_child != nullptr) {                                                       \
                            /* Right child exists and is black - double black */                                       \
                            suc_right_child->color = R_(rbt_double_black);                                             \
                            R_RBT_FIX_DELETE((t), suc_right_child);                                                    \
                        } else {                                                                                       \
                            /* No right child (nullptr) - black leaf removed, fix needed */                            \
                            if (suc_parent_saved == node) {                                                            \
                                /* Successor was direct right child - fix from successor's new position */             \
                                /* The double-black is at node_suc's right (null) */                                   \
                                const bool db_is_right = true; /* Always right for direct right child successor */     \
                                auto sib = node_suc->left;     /* Sibling is on the left */                            \
                                if (sib != nullptr && sib->color == R_(rbt_red)) {                                     \
                                    R_RBT_ROTATE((t), sib, R_(rbt_right));                                             \
                                    sib->color = R_(rbt_black);                                                        \
                                    node_suc->color = R_(rbt_red);                                                     \
                                    sib = node_suc->left;                                                              \
                                }                                                                                      \
                                if (sib != nullptr) {                                                                  \
                                    const bool sl_r = sib->left != nullptr && sib->left->color == R_(rbt_red);         \
                                    const bool sr_r = sib->right != nullptr && sib->right->color == R_(rbt_red);       \
                                    if (!sl_r && !sr_r && node_suc->color == R_(rbt_red)) {                            \
                                        node_suc->color = R_(rbt_black);                                               \
                                        sib->color = R_(rbt_red);                                                      \
                                    } else if (!sl_r && !sr_r && node_suc->color == R_(rbt_black)) {                   \
                                        sib->color = R_(rbt_red);                                                      \
                                        node_suc->color = R_(rbt_double_black);                                        \
                                        R_RBT_FIX_DELETE((t), node_suc);                                               \
                                    } else if ((!db_is_right && sr_r) || (db_is_right && sl_r)) {                      \
                                        const enum rbt_color pc = node_suc->color;                                     \
                                        R_RBT_ROTATE((t), sib, db_is_right ? R_(rbt_right) : R_(rbt_left));            \
                                        sib->color = pc;                                                               \
                                        if (db_is_right) {                                                             \
                                            sib->right->color = R_(rbt_black);                                         \
                                            if (sib->left != nullptr)                                                  \
                                                sib->left->color = R_(rbt_black);                                      \
                                        } else {                                                                       \
                                            sib->left->color = R_(rbt_black);                                          \
                                            if (sib->right != nullptr)                                                 \
                                                sib->right->color = R_(rbt_black);                                     \
                                        }                                                                              \
                                    } else {                                                                           \
                                        auto nr = db_is_right ? sib->right : sib->left;                                \
                                        const enum rbt_color pc2 = node_suc->color;                                    \
                                        R_RBT_ROTATE((t), nr, db_is_right ? R_(rbt_left) : R_(rbt_right));             \
                                        nr->color = R_(rbt_black);                                                     \
                                        nr->parent->color = R_(rbt_red);                                               \
                                        sib = db_is_right ? node_suc->left : node_suc->right;                          \
                                        R_RBT_ROTATE((t), sib, db_is_right ? R_(rbt_right) : R_(rbt_left));            \
                                        sib->color = pc2;                                                              \
                                        sib->left->color = R_(rbt_black);                                              \
                                        sib->right->color = R_(rbt_black);                                             \
                                    }                                                                                  \
                                }                                                                                      \
                            } else {                                                                                   \
                                /* Successor was deeper - fix from saved parent position */                            \
                                const bool suc_was_left = suc_was_left_child;                                          \
                                auto sib = suc_was_left ? suc_parent_saved->right : suc_parent_saved->left;            \
                                /* Apply same black leaf deletion logic */                                             \
                                if (sib != nullptr && sib->color == R_(rbt_red)) {                                     \
                                    R_RBT_ROTATE((t), sib, suc_was_left ? R_(rbt_left) : R_(rbt_right));               \
                                    sib->color = R_(rbt_black);                                                        \
                                    suc_parent_saved->color = R_(rbt_red);                                             \
                                    sib = suc_was_left ? suc_parent_saved->right : suc_parent_saved->left;             \
                                }                                                                                      \
                                if (sib != nullptr) {                                                                  \
                                    const bool sl_r = sib->left != nullptr && sib->left->color == R_(rbt_red);         \
                                    const bool sr_r = sib->right != nullptr && sib->right->color == R_(rbt_red);       \
                                    if (!sl_r && !sr_r && suc_parent_saved->color == R_(rbt_red)) {                    \
                                        suc_parent_saved->color = R_(rbt_black);                                       \
                                        sib->color = R_(rbt_red);                                                      \
                                    } else if (!sl_r && !sr_r && suc_parent_saved->color == R_(rbt_black)) {           \
                                        sib->color = R_(rbt_red);                                                      \
                                        suc_parent_saved->color = R_(rbt_double_black);                                \
                                        R_RBT_FIX_DELETE((t), suc_parent_saved);                                       \
                                    } else if ((suc_was_left && sr_r) || (!suc_was_left && sl_r)) {                    \
                                        const enum rbt_color pc = suc_parent_saved->color;                             \
                                        R_RBT_ROTATE((t), sib, suc_was_left ? R_(rbt_left) : R_(rbt_right));           \
                                        sib->color = pc;                                                               \
                                        if (suc_was_left) {                                                            \
                                            sib->left->color = R_(rbt_black);                                          \
                                            if (sib->right != nullptr)                                                 \
                                                sib->right->color = R_(rbt_black);                                     \
                                        } else {                                                                       \
                                            sib->right->color = R_(rbt_black);                                         \
                                            if (sib->left != nullptr)                                                  \
                                                sib->left->color = R_(rbt_black);                                      \
                                        }                                                                              \
                                    } else {                                                                           \
                                        auto nr = suc_was_left ? sib->left : sib->right;                               \
                                        const enum rbt_color pc2 = suc_parent_saved->color;                            \
                                        R_RBT_ROTATE((t), nr, suc_was_left ? R_(rbt_right) : R_(rbt_left));            \
                                        nr->color = R_(rbt_black);                                                     \
                                        nr->parent->color = R_(rbt_red);                                               \
                                        sib = suc_was_left ? suc_parent_saved->right : suc_parent_saved->left;         \
                                        R_RBT_ROTATE((t), sib, suc_was_left ? R_(rbt_left) : R_(rbt_right));           \
                                        sib->color = pc2;                                                              \
                                        sib->left->color = R_(rbt_black);                                              \
                                        sib->right->color = R_(rbt_black);                                             \
                                    }                                                                                  \
                                }                                                                                      \
                            }                                                                                          \
                        }                                                                                              \
                    } else {                                                                                           \
                        /* One-child case: fix at replacement position */                                              \
                        node_suc->color = R_(rbt_double_black);                                                        \
                        R_RBT_FIX_DELETE((t), node_suc);                                                               \
                    }                                                                                                  \
                } else if (node_color == R_(rbt_black) && node_suc != nullptr && suc_parent_saved == nullptr &&        \
                           suc_color == R_(rbt_red)) {                                                                 \
                    /* One-child case: replacement was red - just color it black */                                    \
                    node_suc->color = R_(rbt_black);                                                                   \
                } else if (node_color == R_(rbt_black) && node_suc == nullptr && parent != nullptr) {                  \
                    /* Black leaf deletion: apply fix from parent's perspective */                                     \
                    /* The sibling is parent's other child */                                                          \
                    const bool del_was_left = is_left_child;                                                           \
                    auto sibling = del_was_left ? parent->right : parent->left;                                        \
                    if (sibling != nullptr && sibling->color == R_(rbt_red)) {                                         \
                        /* Parent must be black, sibling is red: rotate and continue */                                \
                        R_RBT_ROTATE((t), sibling, del_was_left ? R_(rbt_left) : R_(rbt_right));                       \
                        sibling->color = R_(rbt_black);                                                                \
                        parent->color = R_(rbt_red);                                                                   \
                        sibling = del_was_left ? parent->right : parent->left;                                         \
                    }                                                                                                  \
                    if (sibling != nullptr) {                                                                          \
                        const bool sl_red = sibling->left != nullptr && sibling->left->color == R_(rbt_red);           \
                        const bool sr_red = sibling->right != nullptr && sibling->right->color == R_(rbt_red);         \
                        if (!sl_red && !sr_red && parent->color == R_(rbt_red)) {                                      \
                            /* Parent red, sibling black with black children: recolor */                               \
                            parent->color = R_(rbt_black);                                                             \
                            sibling->color = R_(rbt_red);                                                              \
                        } else if (!sl_red && !sr_red && parent->color == R_(rbt_black)) {                             \
                            /* Parent black, sibling black with black children: make sibling red, parent DB */         \
                            sibling->color = R_(rbt_red);                                                              \
                            parent->color = R_(rbt_double_black);                                                      \
                            R_RBT_FIX_DELETE((t), parent);                                                             \
                        } else {                                                                                       \
                            /* Sibling has at least one red child: rotate and recolor */                               \
                            if ((del_was_left && sr_red) || (!del_was_left && sl_red)) {                               \
                                /* Far child is red: final rotation */                                                 \
                                const enum rbt_color pc = parent->color;                                               \
                                if (del_was_left) {                                                                    \
                                    R_RBT_ROTATE((t), sibling, R_(rbt_left));                                          \
                                    sibling->color = pc;                                                               \
                                    sibling->left->color = R_(rbt_black);                                              \
                                    if (sibling->right != nullptr)                                                     \
                                        sibling->right->color = R_(rbt_black);                                         \
                                } else {                                                                               \
                                    R_RBT_ROTATE((t), sibling, R_(rbt_right));                                         \
                                    sibling->color = pc;                                                               \
                                    sibling->right->color = R_(rbt_black);                                             \
                                    if (sibling->left != nullptr)                                                      \
                                        sibling->left->color = R_(rbt_black);                                          \
                                }                                                                                      \
                            } else {                                                                                   \
                                /* Near child is red: rotate to make it far, then rotate again */                      \
                                auto near = del_was_left ? sibling->left : sibling->right;                             \
                                const enum rbt_color parent_color = parent->color;                                     \
                                if (del_was_left) {                                                                    \
                                    R_RBT_ROTATE((t), near, R_(rbt_right));                                            \
                                    near->color = R_(rbt_black);                                                       \
                                    near->parent->color = R_(rbt_red);                                                 \
                                    sibling = parent->right;                                                           \
                                    R_RBT_ROTATE((t), sibling, R_(rbt_left));                                          \
                                    sibling->color = parent_color;                                                     \
                                    sibling->left->color = R_(rbt_black);                                              \
                                    sibling->right->color = R_(rbt_black);                                             \
                                } else {                                                                               \
                                    R_RBT_ROTATE((t), near, R_(rbt_left));                                             \
                                    near->color = R_(rbt_black);                                                       \
                                    near->parent->color = R_(rbt_red);                                                 \
                                    sibling = parent->left;                                                            \
                                    R_RBT_ROTATE((t), sibling, R_(rbt_right));                                         \
                                    sibling->color = parent_color;                                                     \
                                    sibling->right->color = R_(rbt_black);                                             \
                                    sibling->left->color = R_(rbt_black);                                              \
                                }                                                                                      \
                            }                                                                                          \
                        }                                                                                              \
                    }                                                                                                  \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        /* return */ node;                                                                                             \
    })

#endif // RUNE_RBT_API

// Type definition and implementation
// ---------------------------------------------------------------------------------------------------------------------

#ifdef T

struct RBT_NODE(T) {
    T data;
    enum rbt_color color;
    struct RBT_NODE(T) * parent;
    struct RBT_NODE(T) * left;
    struct RBT_NODE(T) * right;
};

typedef struct {
    struct RBT_NODE(T) * root;
    int (*cmp_fn)(T a, T b);
    struct RBT_NODE(T) * (*node_fn)(T data);
    void (*node_free_fn)(struct RBT_NODE(T) * node);
} RBT(T);

static struct RBT_NODE(T) * R_RBT_NODE_NEW(T)(T data) {
    struct RBT_NODE(T) * node = mem_alloc_zero(sizeof(struct RBT_NODE(T)));
    node->color = R_(rbt_red);
    node->data = data;
    return node;
}

static void R_RBT_NODE_FREE(T)(struct RBT_NODE(T) * node) {
    if (node != nullptr) {
        mem_free(node, sizeof(struct RBT_NODE(T)));
    }
}

#endif // T
