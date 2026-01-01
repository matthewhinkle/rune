/**
 * Red-black self-balancing binary search tree.
 *
 * Provides:
 *   - Self-balancing binary search tree with O(log n) operations
 *   - Generic type support via macro-based template expansion
 *   - Custom comparator support per operation with default numeric comparison
 *   - Efficient insertion, search, and deletion operations
 *   - Proper handling of red-black tree invariants
 *
 * Quick Reference:
 *
 *   Red-Black Tree API
 *   -------------------------------------------------------------------------------------------------------------------
 *   rbt(type)                   Create empty tree
 *   rbt_contains(t, val, ...)   Check if value exists (optional comparator)
 *   rbt_insert(t, val, ...)     Insert value, ignored if duplicate (optional comparator)
 *   rbt_remove(t, val, ...)     Remove value from tree (optional comparator)
 *
 *   BST Helper API
 *   -------------------------------------------------------------------------------------------------------------------
 *   bst_min(node)               Find minimum node in subtree
 *   bst_find(t, val, ...)       Find node with value in tree (optional comparator)
 *   bst_remove(t, node)         Remove specific node from tree (internal use)
 *
 * Example:
 *   // Basic tree usage with default numeric comparison
 *   #define T int
 *   #include "tree.h"
 *   RBT(int) tree = rbt(int);
 *   rbt_insert(&tree, 5);
 *   rbt_insert(&tree, 3);
 *   rbt_insert(&tree, 7);
 *   bool found = rbt_contains(&tree, 3);  // true
 *   rbt_remove(&tree, 3);
 *   #undef T
 *
 *   // With custom comparator passed to operations
 *   int cmp(int a, int b) { return (a > b) - (a < b); }
 *   RBT(int) tree2 = rbt(int);
 *   rbt_insert(&tree2, 5, cmp);
 *   rbt_insert(&tree2, 3, cmp);
 *   bool found = rbt_contains(&tree2, 3, cmp);
 *   rbt_remove(&tree2, 5, cmp);
 *
 * Note: RBT requires template expansion via #define T / #undef T for type instantiation.
 */

// ReSharper disable once CppMissingIncludeGuard

#include "r.h"

// Suppress pedantic warnings about GNU statement expressions (intentional, required for macro-based templates)
#pragma GCC diagnostic ignored "-Wpedantic"

// =====================================================================================================================
// Binary Search Tree
// =====================================================================================================================

// API
// ---------------------------------------------------------------------------------------------------------------------

#ifndef RUNE_BST_API
#define RUNE_BST_API

/* R_BST_CMP with optional comparator - selects implementation based on arg count */
#define R_BST_CMP_DEFAULT(a, b) (((a) > (b)) - ((a) < (b)))
#define R_BST_CMP_CUSTOM(a, b, cmp) ((cmp)((a), (b)))
#define R_BST_CMP_SELECT(_1, _2, _3, N, ...) N
#define R_BST_CMP(...) R_BST_CMP_SELECT(__VA_ARGS__, R_BST_CMP_CUSTOM, R_BST_CMP_DEFAULT)(__VA_ARGS__)

#define R_BST_LEFT(node) (node)->parent != nullptr && (node)->parent->left == (node)

#define bst_min(node)                                                                                                  \
    ({                                                                                                                 \
        typeof_unqual((node)) R_UNIQUE(_min_cur) = (node);                                                             \
        while (R_UNIQUE(_min_cur) != nullptr && R_UNIQUE(_min_cur)->left != nullptr) {                                 \
            R_UNIQUE(_min_cur) = R_UNIQUE(_min_cur)->left;                                                             \
        }                                                                                                              \
        /* return */ R_UNIQUE(_min_cur);                                                                               \
    })

#define bst_find(t, val, ...)                                                                                          \
    ({                                                                                                                 \
        typeof_unqual((t)->root) R_UNIQUE(_cur) = (t)->root;                                                           \
        while (R_UNIQUE(_cur) != nullptr && R_BST_CMP((val), R_UNIQUE(_cur)->data __VA_OPT__(, ) __VA_ARGS__) != 0) {  \
            R_UNIQUE(_cur) = R_BST_CMP((val), R_UNIQUE(_cur)->data __VA_OPT__(, ) __VA_ARGS__) < 0 ? R_UNIQUE(_cur)->left \
                                                                                                     : R_UNIQUE(_cur)->right; \
        }                                                                                                              \
        /* return */ R_UNIQUE(_cur);                                                                                   \
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
            if (R_BST_LEFT((node))) {                                                                                  \
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
            typeof_unqual((successor)) R_UNIQUE(_suc_right) = (successor)->right;                                      \
            typeof_unqual((successor)) R_UNIQUE(_suc_parent) = (successor)->parent;                                    \
            /* Replace successor with its right child in its old parent's tree */                                      \
            if (R_UNIQUE(_suc_parent)->left == (successor)) {                                                          \
                R_UNIQUE(_suc_parent)->left = R_UNIQUE(_suc_right);                                                    \
            } else {                                                                                                   \
                R_UNIQUE(_suc_parent)->right = R_UNIQUE(_suc_right);                                                   \
            }                                                                                                          \
            /* Update right child's parent if it exists */                                                             \
            if (R_UNIQUE(_suc_right) != nullptr) {                                                                     \
                R_UNIQUE(_suc_right)->parent = R_UNIQUE(_suc_parent);                                                  \
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
            if (R_BST_LEFT((node))) {                                                                                  \
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
        typeof_unqual((node)) suc = nullptr;                                                                           \
        typeof_unqual((node)) node_to_free = (node);                                                                   \
        if ((node)->left == nullptr) {                                                                                 \
            suc = (node)->right;                                                                                       \
            R_BST_REPLACE((t), (node), (node)->right);                                                                 \
        } else if ((node)->right == nullptr) {                                                                         \
            suc = (node)->left;                                                                                        \
            R_BST_REPLACE((t), (node), (node)->left);                                                                  \
        } else {                                                                                                       \
            /* node has left and right children */                                                                     \
            typeof_unqual((node)->right) min_right = bst_min((node)->right);                                           \
            R_BST_SUCCEED((t), (node), min_right);                                                                     \
            suc = min_right;                                                                                           \
        }                                                                                                              \
        /* Clear node's pointers before freeing to prevent dangling references */                                      \
        node_to_free->left = nullptr;                                                                                  \
        node_to_free->right = nullptr;                                                                                 \
        node_to_free->parent = nullptr;                                                                                \
        mem_free(node_to_free, (t)->node_size);                                                                        \
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

#define rbt(type) {.root = nullptr, .node_size = sizeof(struct RBT_NODE(type)), .size = 0}

#define R_RBT_PARENT(t, val, out_dir, ...)                                                                             \
    ({                                                                                                                 \
        typeof_unqual((t)->root) R_UNIQUE(_parent_parent) = nullptr;                                                   \
        typeof_unqual((t)->root) R_UNIQUE(_parent_cur) = (t)->root;                                                    \
        int R_UNIQUE(_parent_cmp) = 0;                                                                                \
        while (R_UNIQUE(_parent_cur) != nullptr &&                                                                     \
               (R_UNIQUE(_parent_cmp) = R_BST_CMP((val), R_UNIQUE(_parent_cur)->data __VA_OPT__(, ) __VA_ARGS__)) != 0) { \
            R_UNIQUE(_parent_parent) = R_UNIQUE(_parent_cur);                                                          \
            R_UNIQUE(_parent_cur) = R_UNIQUE(_parent_cmp) < 0 ? R_UNIQUE(_parent_cur)->left : R_UNIQUE(_parent_cur)->right; \
        }                                                                                                              \
        if ((out_dir) != nullptr) {                                                                                    \
            if (R_UNIQUE(_parent_cur) == nullptr) {                                                                    \
                *(out_dir) = R_UNIQUE(_parent_cmp) < 0 ? R_(rbt_left) : R_(rbt_right);                                 \
            } else {                                                                                                   \
                *(out_dir) = R_(rbt_exists);                                                                           \
            }                                                                                                          \
        }                                                                                                              \
                                                                                                                       \
        /* return */ R_UNIQUE(_parent_parent);                                                                         \
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
        typeof_unqual((node)->parent) R_UNIQUE(_node_parent) = (node)->parent;                                         \
        if (R_UNIQUE(_node_parent) != nullptr) {                                                                       \
            /* cannot rotate root */                                                                                   \
                                                                                                                       \
            /* update the parent pointers */                                                                           \
            const typeof_unqual(R_UNIQUE(_node_parent)->parent) R_UNIQUE(_gparent) = R_UNIQUE(_node_parent)->parent;   \
            if (R_UNIQUE(_gparent) == nullptr) {                                                                       \
                /* parent is root */                                                                                   \
                t->root = (node);                                                                                      \
            } else if (R_BST_LEFT(R_UNIQUE(_node_parent))) {                                                            \
                R_UNIQUE(_gparent)->left = (node);                                                                     \
            } else {                                                                                                   \
                R_UNIQUE(_gparent)->right = (node);                                                                    \
            }                                                                                                          \
            (node)->parent = R_UNIQUE(_node_parent)->parent;                                                           \
                                                                                                                       \
            /* node make node's former parent its right child */                                                       \
            typeof_unqual((t)->root) R_UNIQUE(_prev) = nullptr;                                                        \
            if (dir == R_(rbt_left)) {                                                                                 \
                R_UNIQUE(_prev) = (node)->left;                                                                        \
                (node)->left = R_UNIQUE(_node_parent);                                                                 \
                R_UNIQUE(_node_parent)->parent = (node);                                                               \
                R_UNIQUE(_node_parent)->right = R_UNIQUE(_prev);                                                       \
            } else {                                                                                                   \
                R_UNIQUE(_prev) = node->right;                                                                         \
                node->right = R_UNIQUE(_node_parent);                                                                  \
                R_UNIQUE(_node_parent)->parent = node;                                                                 \
                R_UNIQUE(_node_parent)->left = R_UNIQUE(_prev);                                                        \
            }                                                                                                          \
                                                                                                                       \
            if (R_UNIQUE(_prev) != nullptr) {                                                                          \
                R_UNIQUE(_prev)->parent = R_UNIQUE(_node_parent);                                                      \
            }                                                                                                          \
        }                                                                                                              \
    })

/**
 * Get the color of a node's sibling.
 * Assumes parent is not null. Treats null siblings as black.
 */
#define R_RBT_SIB_COLOR(node)                                                                                          \
    (R_BST_LEFT((node)) ? ((node)->parent->right != nullptr ? (node)->parent->right->color : R_(rbt_black))            \
                        : ((node)->parent->left != nullptr ? (node)->parent->left->color : R_(rbt_black)))

/* rbt_contains: optional comparator */
#define rbt_contains(t, val, ...) (((t) != nullptr) && (bst_find((t), (val), __VA_ARGS__) != nullptr))

#define rbt_insert(t, val, ...)                                                                                        \
    ({                                                                                                                 \
        if ((t) != nullptr) {                                                                                          \
            /* 1 - find the parent and direction */                                                                    \
            enum rbt_dir dir = R_(rbt_exists);                                                                         \
            typeof_unqual((t)->root) parent = R_RBT_PARENT((t), (val), &dir __VA_OPT__(, ) __VA_ARGS__);               \
                                                                                                                       \
            /* 2 - check if the node already exists */                                                                 \
            if (dir != R_(rbt_exists)) {                                                                               \
                /* 3 - create the new node */                                                                          \
                typeof_unqual((t)->root) new_node = mem_alloc_zero((t)->node_size);                                    \
                new_node->color = R_(rbt_red);                                                                         \
                new_node->data = (val);                                                                                \
                (t)->size++;                                                                                           \
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
                    typeof_unqual((t)->root) cur = new_node;                                                           \
                                                                                                                       \
                    while (cur->parent != nullptr && cur->parent->color == R_(rbt_red)) {                              \
                        /* 7 - parent is red, check the uncle; leaf (null) nodes are considered black */               \
                        const bool parent_left = R_BST_LEFT(cur->parent);                                              \
                        typeof_unqual((t)->root) gparent = cur->parent->parent;                                        \
                        typeof_unqual((t)->root) uncle = parent_left ? gparent->right : gparent->left;                 \
                                                                                                                       \
                        /* 8 - check if uncle is black, if so, we need rotations and then exit */                      \
                        if (uncle == nullptr || uncle->color == R_(rbt_black)) {                                       \
                            /* uncle is black */                                                                       \
                            if (R_BST_LEFT(cur)) {                                                                     \
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
            const bool R_UNIQUE(_fix_is_left) = R_BST_LEFT((node));                                                    \
            typeof_unqual((node)->parent) R_UNIQUE(_fix_sib) = R_UNIQUE(_fix_is_left) ? (node)->parent->right : (node)->parent->left; \
                                                                                                                       \
            if (R_UNIQUE(_fix_sib) != nullptr && R_UNIQUE(_fix_sib)->color == R_(rbt_red)) {                           \
                /* CASE 1: Sibling is RED - rotate and recolor to convert to cases 2-4 */                              \
                const enum rbt_color R_UNIQUE(_fix_pcolor) = (node)->parent->color;                                    \
                if (R_UNIQUE(_fix_is_left)) {                                                                          \
                    R_RBT_ROTATE((t), R_UNIQUE(_fix_sib), R_(rbt_left));                                               \
                } else {                                                                                               \
                    R_RBT_ROTATE((t), R_UNIQUE(_fix_sib), R_(rbt_right));                                              \
                }                                                                                                      \
                R_UNIQUE(_fix_sib)->color = R_UNIQUE(_fix_pcolor);                                                     \
                (node)->parent->color = R_(rbt_red);                                                                   \
                /* Get new sibling after rotation (was child of old sibling) */                                        \
                R_UNIQUE(_fix_sib) = R_UNIQUE(_fix_is_left) ? (node)->parent->right : (node)->parent->left;            \
            }                                                                                                          \
                                                                                                                       \
            if (R_UNIQUE(_fix_sib) != nullptr) {                                                                       \
                const bool R_UNIQUE(_fix_sl_red) = R_UNIQUE(_fix_sib)->left != nullptr && R_UNIQUE(_fix_sib)->left->color == R_(rbt_red); \
                const bool R_UNIQUE(_fix_sr_red) = R_UNIQUE(_fix_sib)->right != nullptr && R_UNIQUE(_fix_sib)->right->color == R_(rbt_red); \
                                                                                                                       \
                if (!R_UNIQUE(_fix_sl_red) && !R_UNIQUE(_fix_sr_red)) {                                                \
                    /* CASE 2: Sibling BLACK, both children BLACK */                                                   \
                    R_UNIQUE(_fix_sib)->color = R_(rbt_red);                                                           \
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
                } else if ((R_UNIQUE(_fix_is_left) && R_UNIQUE(_fix_sr_red)) || (!R_UNIQUE(_fix_is_left) && R_UNIQUE(_fix_sl_red))) { \
                    /* CASE 4: Far child RED - final fix eliminates double-black */                                    \
                    if (R_UNIQUE(_fix_is_left)) {                                                                      \
                        const enum rbt_color R_UNIQUE(_fix_pcolor2) = (node)->parent->color;                           \
                        R_RBT_ROTATE((t), R_UNIQUE(_fix_sib), R_(rbt_left));                                           \
                        R_UNIQUE(_fix_sib)->color = R_UNIQUE(_fix_pcolor2);                                            \
                        R_UNIQUE(_fix_sib)->left->color = R_(rbt_black);                                               \
                        R_UNIQUE(_fix_sib)->right->color = R_(rbt_black);                                              \
                    } else {                                                                                           \
                        const enum rbt_color R_UNIQUE(_fix_pcolor2) = (node)->parent->color;                           \
                        R_RBT_ROTATE((t), R_UNIQUE(_fix_sib), R_(rbt_right));                                          \
                        R_UNIQUE(_fix_sib)->color = R_UNIQUE(_fix_pcolor2);                                            \
                        R_UNIQUE(_fix_sib)->left->color = R_(rbt_black);                                               \
                        R_UNIQUE(_fix_sib)->right->color = R_(rbt_black);                                              \
                    }                                                                                                  \
                    (node)->color = R_(rbt_black);                                                                     \
                    (node) = nullptr; /* Exit loop - violation resolved */                                             \
                } else {                                                                                               \
                    /* CASE 3: Near child RED, far child BLACK - setup for case 4 */                                   \
                    typeof_unqual(R_UNIQUE(_fix_sib)) R_UNIQUE(_fix_near) = R_UNIQUE(_fix_is_left) ? R_UNIQUE(_fix_sib)->left : R_UNIQUE(_fix_sib)->right; \
                    if (R_UNIQUE(_fix_is_left)) {                                                                      \
                        R_RBT_ROTATE((t), R_UNIQUE(_fix_near), R_(rbt_right));                                         \
                    } else {                                                                                           \
                        R_RBT_ROTATE((t), R_UNIQUE(_fix_near), R_(rbt_left));                                          \
                    }                                                                                                  \
                    /* After rotation, get new sibling */                                                              \
                    R_UNIQUE(_fix_sib) = R_UNIQUE(_fix_is_left) ? (node)->parent->right : (node)->parent->left;        \
                    R_UNIQUE(_fix_sib)->color = R_(rbt_black);                                                         \
                    /* Old sibling is now a child of new sibling - color it red */                                     \
                    typeof_unqual(R_UNIQUE(_fix_sib)) R_UNIQUE(_fix_old_sib) = R_UNIQUE(_fix_is_left) ? R_UNIQUE(_fix_sib)->right : R_UNIQUE(_fix_sib)->left; \
                    R_UNIQUE(_fix_old_sib)->color = R_(rbt_red);                                                       \
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

#define rbt_remove(t, val, ...)                                                                                        \
    ({                                                                                                                 \
        typeof_unqual((t)->root) R_UNIQUE(_remove_node) = nullptr;                                                    \
        if ((t) != nullptr) {                                                                                          \
            R_UNIQUE(_remove_node) = bst_find((t), (val), __VA_ARGS__);                                               \
            if (R_UNIQUE(_remove_node) != nullptr) {                                                                  \
                (t)->size--;                                                                                           \
                const enum rbt_color R_UNIQUE(_remove_node_color) = R_UNIQUE(_remove_node)->color;                    \
                typeof_unqual((t)->root) R_UNIQUE(_remove_parent) = R_UNIQUE(_remove_node)->parent;                   \
                const bool R_UNIQUE(_remove_is_left_child) = R_UNIQUE(_remove_parent) != nullptr && R_BST_LEFT(R_UNIQUE(_remove_node)); \
                /* For two-children case, save successor's right child (will replace successor) */                     \
                typeof_unqual((t)->root) R_UNIQUE(_remove_suc_right_child) = nullptr;                                  \
                typeof_unqual((t)->root) R_UNIQUE(_remove_suc_parent_saved) = nullptr;                                 \
                bool R_UNIQUE(_remove_suc_was_left_child) = false;                                                    \
                typeof_unqual((t)->root) R_UNIQUE(_remove_successor) = nullptr;                                        \
                if (R_UNIQUE(_remove_node)->left != nullptr && R_UNIQUE(_remove_node)->right != nullptr) {             \
                    R_UNIQUE(_remove_successor) = bst_min(R_UNIQUE(_remove_node)->right);                             \
                    R_UNIQUE(_remove_suc_right_child) = R_UNIQUE(_remove_successor)->right;                           \
                    R_UNIQUE(_remove_suc_parent_saved) = R_UNIQUE(_remove_successor)->parent;                         \
                    R_UNIQUE(_remove_suc_was_left_child) = R_UNIQUE(_remove_successor)->parent != nullptr && R_BST_LEFT(R_UNIQUE(_remove_successor)); \
                }                                                                                                      \
                const enum rbt_color R_UNIQUE(_remove_suc_color) =                                                    \
                    (R_UNIQUE(_remove_node)->left == nullptr && R_UNIQUE(_remove_node)->right == nullptr)             \
                        ? R_(rbt_black)                                                                                \
                        : (R_UNIQUE(_remove_node)->left == nullptr                                                    \
                               ? R_UNIQUE(_remove_node)->right->color                                                 \
                               : (R_UNIQUE(_remove_node)->right == nullptr ? R_UNIQUE(_remove_node)->left->color : R_UNIQUE(_remove_successor)->color)); \
                typeof_unqual((t)->root) R_UNIQUE(_remove_node_suc) = bst_remove((t), (R_UNIQUE(_remove_node)));                                           \
                /* Only fix if both deleted node and replacement were black */                                         \
                if (R_UNIQUE(_remove_node_color) == R_(rbt_black) && R_UNIQUE(_remove_suc_color) == R_(rbt_black) && R_UNIQUE(_remove_node_suc) != nullptr) {                \
                    if (R_UNIQUE(_remove_suc_right_child) != nullptr || R_UNIQUE(_remove_suc_parent_saved) != nullptr) {                                   \
                        /* Two-children case: fix at successor's old position */                                       \
                        const enum rbt_color R_UNIQUE(_remove_rc_color) =                                                                \
                            R_UNIQUE(_remove_suc_right_child) != nullptr ? R_UNIQUE(_remove_suc_right_child)->color : R_(rbt_black);                       \
                        if (R_UNIQUE(_remove_rc_color) == R_(rbt_red)) {                                                                 \
                            /* Right child was red - just color it black */                                            \
                            R_UNIQUE(_remove_suc_right_child)->color = R_(rbt_black);                                                    \
                        } else if (R_UNIQUE(_remove_suc_right_child) != nullptr) {                                                       \
                            /* Right child exists and is black - double black */                                       \
                            R_UNIQUE(_remove_suc_right_child)->color = R_(rbt_double_black);                                             \
                            R_RBT_FIX_DELETE((t), R_UNIQUE(_remove_suc_right_child));                                                    \
                        } else {                                                                                       \
                            /* No right child (nullptr) - black leaf removed, fix needed */                            \
                            if (R_UNIQUE(_remove_suc_parent_saved) == R_UNIQUE(_remove_node)) {                                                            \
                                /* Successor was direct right child - fix from successor's new position */             \
                                /* The double-black is at R_UNIQUE(_remove_node_suc)'s right (null) */                                   \
                                const bool R_UNIQUE(_remove_db_is_right) = true; /* Always right for direct right child successor */     \
                                typeof_unqual(R_UNIQUE(_remove_node_suc)) R_UNIQUE(_remove_sib_direct) = R_UNIQUE(_remove_node_suc)->left; /* Sibling is on the left */      \
                                if (R_UNIQUE(_remove_sib_direct) != nullptr && R_UNIQUE(_remove_sib_direct)->color == R_(rbt_red)) {                       \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_direct), R_(rbt_right));                                      \
                                    R_UNIQUE(_remove_sib_direct)->color = R_(rbt_black);                                                 \
                                    R_UNIQUE(_remove_node_suc)->color = R_(rbt_red);                                                     \
                                    R_UNIQUE(_remove_sib_direct) = R_UNIQUE(_remove_node_suc)->left;                                                       \
                                }                                                                                      \
                                if (R_UNIQUE(_remove_sib_direct) != nullptr) {                                                           \
                                    const bool R_UNIQUE(_remove_sl_r_direct) = R_UNIQUE(_remove_sib_direct)->left != nullptr && R_UNIQUE(_remove_sib_direct)->left->color == R_(rbt_red); \
                                    const bool R_UNIQUE(_remove_sr_r_direct) = R_UNIQUE(_remove_sib_direct)->right != nullptr && R_UNIQUE(_remove_sib_direct)->right->color == R_(rbt_red); \
                                    if (!R_UNIQUE(_remove_sl_r_direct) && !R_UNIQUE(_remove_sr_r_direct) && R_UNIQUE(_remove_node_suc)->color == R_(rbt_red)) {              \
                                        R_UNIQUE(_remove_node_suc)->color = R_(rbt_black);                                               \
                                        R_UNIQUE(_remove_sib_direct)->color = R_(rbt_red);                                               \
                                    } else if (!R_UNIQUE(_remove_sl_r_direct) && !R_UNIQUE(_remove_sr_r_direct) && R_UNIQUE(_remove_node_suc)->color == R_(rbt_black)) {     \
                                        R_UNIQUE(_remove_sib_direct)->color = R_(rbt_red);                                               \
                                        R_UNIQUE(_remove_node_suc)->color = R_(rbt_double_black);                                        \
                                        R_RBT_FIX_DELETE((t), R_UNIQUE(_remove_node_suc));                                               \
                                    } else if ((!R_UNIQUE(_remove_db_is_right) && R_UNIQUE(_remove_sr_r_direct)) || (R_UNIQUE(_remove_db_is_right) && R_UNIQUE(_remove_sl_r_direct))) {        \
                                        const enum rbt_color R_UNIQUE(_remove_pc_direct) = R_UNIQUE(_remove_node_suc)->color;                              \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_direct), R_UNIQUE(_remove_db_is_right) ? R_(rbt_right) : R_(rbt_left));    \
                                        R_UNIQUE(_remove_sib_direct)->color = R_UNIQUE(_remove_pc_direct);                                                 \
                                        if (R_UNIQUE(_remove_db_is_right)) {                                                             \
                                            R_UNIQUE(_remove_sib_direct)->right->color = R_(rbt_black);                                  \
                                            if (R_UNIQUE(_remove_sib_direct)->left != nullptr)                                           \
                                                R_UNIQUE(_remove_sib_direct)->left->color = R_(rbt_black);                               \
                                        } else {                                                                       \
                                            R_UNIQUE(_remove_sib_direct)->left->color = R_(rbt_black);                                   \
                                            if (R_UNIQUE(_remove_sib_direct)->right != nullptr)                                          \
                                                R_UNIQUE(_remove_sib_direct)->right->color = R_(rbt_black);                              \
                                        }                                                                              \
                                    } else {                                                                           \
                                        typeof_unqual(R_UNIQUE(_remove_sib_direct)) R_UNIQUE(_remove_nr_direct) = R_UNIQUE(_remove_db_is_right) ? R_UNIQUE(_remove_sib_direct)->right : R_UNIQUE(_remove_sib_direct)->left; \
                                        const enum rbt_color R_UNIQUE(_remove_pc2_direct) = R_UNIQUE(_remove_node_suc)->color;                             \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_nr_direct), R_UNIQUE(_remove_db_is_right) ? R_(rbt_left) : R_(rbt_right));      \
                                        R_UNIQUE(_remove_nr_direct)->color = R_(rbt_black);                                              \
                                        R_UNIQUE(_remove_nr_direct)->parent->color = R_(rbt_red);                                        \
                                        R_UNIQUE(_remove_sib_direct) = R_UNIQUE(_remove_db_is_right) ? R_UNIQUE(_remove_node_suc)->left : R_UNIQUE(_remove_node_suc)->right;                  \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_direct), R_UNIQUE(_remove_db_is_right) ? R_(rbt_right) : R_(rbt_left));    \
                                        R_UNIQUE(_remove_sib_direct)->color = R_UNIQUE(_remove_pc2_direct);                                                \
                                        R_UNIQUE(_remove_sib_direct)->left->color = R_(rbt_black);                                       \
                                        R_UNIQUE(_remove_sib_direct)->right->color = R_(rbt_black);                                      \
                                    }                                                                                  \
                                }                                                                                      \
                            } else {                                                                                   \
                                /* Successor was deeper - fix from saved R_UNIQUE(_remove_parent) position */                            \
                                const bool R_UNIQUE(_remove_suc_was_left) = R_UNIQUE(_remove_suc_was_left_child);                                          \
                                typeof_unqual(R_UNIQUE(_remove_suc_parent_saved)) R_UNIQUE(_remove_sib_suc) =                                              \
                                    R_UNIQUE(_remove_suc_was_left) ? R_UNIQUE(_remove_suc_parent_saved)->right : R_UNIQUE(_remove_suc_parent_saved)->left;                   \
                                /* Apply same black leaf deletion logic */                                             \
                                if (R_UNIQUE(_remove_sib_suc) != nullptr && R_UNIQUE(_remove_sib_suc)->color == R_(rbt_red)) {                             \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_suc), R_UNIQUE(_remove_suc_was_left) ? R_(rbt_left) : R_(rbt_right));           \
                                    R_UNIQUE(_remove_sib_suc)->color = R_(rbt_black);                                                    \
                                    R_UNIQUE(_remove_suc_parent_saved)->color = R_(rbt_red);                                             \
                                    R_UNIQUE(_remove_sib_suc) = R_UNIQUE(_remove_suc_was_left) ? R_UNIQUE(_remove_suc_parent_saved)->right : R_UNIQUE(_remove_suc_parent_saved)->left;         \
                                }                                                                                      \
                                if (R_UNIQUE(_remove_sib_suc) != nullptr) {                                                              \
                                    const bool R_UNIQUE(_remove_sl_r_suc) = R_UNIQUE(_remove_sib_suc)->left != nullptr && R_UNIQUE(_remove_sib_suc)->left->color == R_(rbt_red); \
                                    const bool R_UNIQUE(_remove_sr_r_suc) = R_UNIQUE(_remove_sib_suc)->right != nullptr && R_UNIQUE(_remove_sib_suc)->right->color == R_(rbt_red);       \
                                    if (!R_UNIQUE(_remove_sl_r_suc) && !R_UNIQUE(_remove_sr_r_suc) && R_UNIQUE(_remove_suc_parent_saved)->color == R_(rbt_red)) { \
                                        R_UNIQUE(_remove_suc_parent_saved)->color = R_(rbt_black);                                       \
                                        R_UNIQUE(_remove_sib_suc)->color = R_(rbt_red);                                                  \
                                    } else if (!R_UNIQUE(_remove_sl_r_suc) && !R_UNIQUE(_remove_sr_r_suc) && R_UNIQUE(_remove_suc_parent_saved)->color == R_(rbt_black)) {  \
                                        R_UNIQUE(_remove_sib_suc)->color = R_(rbt_red);                                                  \
                                        R_UNIQUE(_remove_suc_parent_saved)->color = R_(rbt_double_black);                                \
                                        R_RBT_FIX_DELETE((t), R_UNIQUE(_remove_suc_parent_saved));                                       \
                                    } else if ((R_UNIQUE(_remove_suc_was_left) && R_UNIQUE(_remove_sr_r_suc)) || (!R_UNIQUE(_remove_suc_was_left) && R_UNIQUE(_remove_sl_r_suc))) {            \
                                        const enum rbt_color R_UNIQUE(_remove_pc_suc) = R_UNIQUE(_remove_suc_parent_saved)->color;                         \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_suc), R_UNIQUE(_remove_suc_was_left) ? R_(rbt_left) : R_(rbt_right));       \
                                        R_UNIQUE(_remove_sib_suc)->color = R_UNIQUE(_remove_pc_suc);                                                       \
                                        if (R_UNIQUE(_remove_suc_was_left)) {                                                            \
                                            R_UNIQUE(_remove_sib_suc)->left->color = R_(rbt_black);                                      \
                                            if (R_UNIQUE(_remove_sib_suc)->right != nullptr)                                             \
                                                R_UNIQUE(_remove_sib_suc)->right->color = R_(rbt_black);                                 \
                                        } else {                                                                       \
                                            R_UNIQUE(_remove_sib_suc)->right->color = R_(rbt_black);                                     \
                                            if (R_UNIQUE(_remove_sib_suc)->left != nullptr)                                              \
                                                R_UNIQUE(_remove_sib_suc)->left->color = R_(rbt_black);                                  \
                                        }                                                                              \
                                    } else {                                                                           \
                                        typeof_unqual(R_UNIQUE(_remove_sib_suc)) R_UNIQUE(_remove_nr_suc) = R_UNIQUE(_remove_suc_was_left) ? R_UNIQUE(_remove_sib_suc)->left : R_UNIQUE(_remove_sib_suc)->right; \
                                        const enum rbt_color R_UNIQUE(_remove_pc2_suc) = R_UNIQUE(_remove_suc_parent_saved)->color;                        \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_nr_suc), R_UNIQUE(_remove_suc_was_left) ? R_(rbt_right) : R_(rbt_left));        \
                                        R_UNIQUE(_remove_nr_suc)->color = R_(rbt_black);                                                 \
                                        R_UNIQUE(_remove_nr_suc)->parent->color = R_(rbt_red);                                           \
                                        R_UNIQUE(_remove_sib_suc) = R_UNIQUE(_remove_suc_was_left) ? R_UNIQUE(_remove_suc_parent_saved)->right : R_UNIQUE(_remove_suc_parent_saved)->left;     \
                                        R_RBT_ROTATE((t), R_UNIQUE(_remove_sib_suc), R_UNIQUE(_remove_suc_was_left) ? R_(rbt_left) : R_(rbt_right));       \
                                        R_UNIQUE(_remove_sib_suc)->color = R_UNIQUE(_remove_pc2_suc);                                                      \
                                        R_UNIQUE(_remove_sib_suc)->left->color = R_(rbt_black);                                          \
                                        R_UNIQUE(_remove_sib_suc)->right->color = R_(rbt_black);                                             \
                                    }                                                                                  \
                                }                                                                                      \
                            }                                                                                          \
                        }                                                                                              \
                    } else {                                                                                           \
                        /* One-child case: fix at replacement position */                                              \
                        R_UNIQUE(_remove_node_suc)->color = R_(rbt_double_black);                                                        \
                        R_RBT_FIX_DELETE((t), R_UNIQUE(_remove_node_suc));                                                               \
                    }                                                                                                  \
                } else if (R_UNIQUE(_remove_node_color) == R_(rbt_black) && R_UNIQUE(_remove_node_suc) != nullptr && R_UNIQUE(_remove_suc_parent_saved) == nullptr &&        \
                           R_UNIQUE(_remove_suc_color) == R_(rbt_red)) {                                                                 \
                    /* One-child case: replacement was red - just color it black */                                    \
                    R_UNIQUE(_remove_node_suc)->color = R_(rbt_black);                                                                   \
                } else if (R_UNIQUE(_remove_node_color) == R_(rbt_black) && R_UNIQUE(_remove_node_suc) == nullptr && R_UNIQUE(_remove_parent) != nullptr) {                  \
                    /* Black leaf deletion: apply fix from R_UNIQUE(_remove_parent)'s perspective */                                     \
                    /* The R_UNIQUE(_remove_sibling) is R_UNIQUE(_remove_parent)'s other child */                                                          \
                    const bool R_UNIQUE(_remove_del_was_left) = R_UNIQUE(_remove_is_left_child);                                                           \
                    typeof_unqual(R_UNIQUE(_remove_parent)) R_UNIQUE(_remove_sibling) = R_UNIQUE(_remove_del_was_left) ? R_UNIQUE(_remove_parent)->right : R_UNIQUE(_remove_parent)->left;                       \
                    if (R_UNIQUE(_remove_sibling) != nullptr && R_UNIQUE(_remove_sibling)->color == R_(rbt_red)) {                                         \
                        /* Parent must be black, R_UNIQUE(_remove_sibling) is red: rotate and continue */                                \
                        R_RBT_ROTATE((t), R_UNIQUE(_remove_sibling), R_UNIQUE(_remove_del_was_left) ? R_(rbt_left) : R_(rbt_right));                       \
                        R_UNIQUE(_remove_sibling)->color = R_(rbt_black);                                                                \
                        R_UNIQUE(_remove_parent)->color = R_(rbt_red);                                                                   \
                        R_UNIQUE(_remove_sibling) = R_UNIQUE(_remove_del_was_left) ? R_UNIQUE(_remove_parent)->right : R_UNIQUE(_remove_parent)->left;                                         \
                    }                                                                                                  \
                    if (R_UNIQUE(_remove_sibling) != nullptr) {                                                                          \
                        const bool R_UNIQUE(_remove_sl_red) = R_UNIQUE(_remove_sibling)->left != nullptr && R_UNIQUE(_remove_sibling)->left->color == R_(rbt_red);           \
                        const bool R_UNIQUE(_remove_sr_red) = R_UNIQUE(_remove_sibling)->right != nullptr && R_UNIQUE(_remove_sibling)->right->color == R_(rbt_red);         \
                        if (!R_UNIQUE(_remove_sl_red) && !R_UNIQUE(_remove_sr_red) && R_UNIQUE(_remove_parent)->color == R_(rbt_red)) {                                      \
                            /* Parent red, R_UNIQUE(_remove_sibling) black with black children: recolor */                               \
                            R_UNIQUE(_remove_parent)->color = R_(rbt_black);                                                             \
                            R_UNIQUE(_remove_sibling)->color = R_(rbt_red);                                                              \
                        } else if (!R_UNIQUE(_remove_sl_red) && !R_UNIQUE(_remove_sr_red) && R_UNIQUE(_remove_parent)->color == R_(rbt_black)) {                             \
                            /* Parent black, R_UNIQUE(_remove_sibling) black with black children: make R_UNIQUE(_remove_sibling) red, R_UNIQUE(_remove_parent) DB */         \
                            R_UNIQUE(_remove_sibling)->color = R_(rbt_red);                                                              \
                            R_UNIQUE(_remove_parent)->color = R_(rbt_double_black);                                                      \
                            R_RBT_FIX_DELETE((t), R_UNIQUE(_remove_parent));                                                             \
                        } else {                                                                                       \
                            /* Sibling has at least one red child: rotate and recolor */                               \
                            if ((R_UNIQUE(_remove_del_was_left) && R_UNIQUE(_remove_sr_red)) || (!R_UNIQUE(_remove_del_was_left) && R_UNIQUE(_remove_sl_red))) {                               \
                                /* Far child is red: final rotation */                                                 \
                                const enum rbt_color R_UNIQUE(_remove_pc_leaf) = R_UNIQUE(_remove_parent)->color;                                          \
                                if (R_UNIQUE(_remove_del_was_left)) {                                                                    \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sibling), R_(rbt_left));                                          \
                                    R_UNIQUE(_remove_sibling)->color = R_UNIQUE(_remove_pc_leaf);                                                          \
                                    R_UNIQUE(_remove_sibling)->left->color = R_(rbt_black);                                              \
                                    if (R_UNIQUE(_remove_sibling)->right != nullptr)                                                     \
                                        R_UNIQUE(_remove_sibling)->right->color = R_(rbt_black);                                         \
                                } else {                                                                               \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sibling), R_(rbt_right));                                         \
                                    R_UNIQUE(_remove_sibling)->color = R_UNIQUE(_remove_pc_leaf);                                                               \
                                    R_UNIQUE(_remove_sibling)->right->color = R_(rbt_black);                                             \
                                    if (R_UNIQUE(_remove_sibling)->left != nullptr)                                                      \
                                        R_UNIQUE(_remove_sibling)->left->color = R_(rbt_black);                                          \
                                }                                                                                      \
                            } else {                                                                                   \
                                /* Near child is red: rotate to make it far, then rotate again */                      \
                                typeof_unqual(R_UNIQUE(_remove_sibling)) R_UNIQUE(_remove_near) = R_UNIQUE(_remove_del_was_left) ? R_UNIQUE(_remove_sibling)->left : R_UNIQUE(_remove_sibling)->right;           \
                                const enum rbt_color R_UNIQUE(_remove_parent_color) = R_UNIQUE(_remove_parent)->color;                                     \
                                if (R_UNIQUE(_remove_del_was_left)) {                                                                    \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_near), R_(rbt_right));                                            \
                                    R_UNIQUE(_remove_near)->color = R_(rbt_black);                                                       \
                                    R_UNIQUE(_remove_near)->parent->color = R_(rbt_red);                                                 \
                                    R_UNIQUE(_remove_sibling) = R_UNIQUE(_remove_parent)->right;                                                           \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sibling), R_(rbt_left));                                          \
                                    R_UNIQUE(_remove_sibling)->color = R_UNIQUE(_remove_parent_color);                                                     \
                                    R_UNIQUE(_remove_sibling)->left->color = R_(rbt_black);                                              \
                                    R_UNIQUE(_remove_sibling)->right->color = R_(rbt_black);                                             \
                                } else {                                                                               \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_near), R_(rbt_left));                                             \
                                    R_UNIQUE(_remove_near)->color = R_(rbt_black);                                                       \
                                    R_UNIQUE(_remove_near)->parent->color = R_(rbt_red);                                                 \
                                    R_UNIQUE(_remove_sibling) = R_UNIQUE(_remove_parent)->left;                                                            \
                                    R_RBT_ROTATE((t), R_UNIQUE(_remove_sibling), R_(rbt_right));                                         \
                                    R_UNIQUE(_remove_sibling)->color = R_UNIQUE(_remove_parent_color);                                                     \
                                    R_UNIQUE(_remove_sibling)->right->color = R_(rbt_black);                                             \
                                    R_UNIQUE(_remove_sibling)->left->color = R_(rbt_black);                                              \
                                }                                                                                      \
                            }                                                                                          \
                        }                                                                                              \
                    }                                                                                                  \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        /* return */ R_UNIQUE(_remove_node);                                                                                             \
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
    const size_t node_size;
    size_t size;
} RBT(T);

#endif // T
