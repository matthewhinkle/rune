#if defined(__cplusplus)
#error "This file must be compiled as C, not C++"
#endif // no cpp assert

#include <stdio.h>

#include "coll.h"

void test_red_uncle_case() {
    printf("=== Test: Red Uncle Case ===\n");
    printf("Inserting: 50(B), 30(R), 70(B), 20(R)\n");
    printf("Expected: After inserting 20, uncle 70 is red, triggering recolor\n\n");

    rb_tree tree = {.root = nullptr};

    rb_insert(&tree, 50);
    printf("After insert 50:\n");
    rb_print(tree.root);
    printf("\n");

    rb_insert(&tree, 30);
    printf("After insert 30 (becomes red, 50 is parent/root):\n");
    rb_print(tree.root);
    printf("\n");

    rb_insert(&tree, 70);
    printf("After insert 70 (becomes red):\n");
    rb_print(tree.root);
    printf("\n");

    rb_insert(&tree, 20);
    printf("After insert 20 (triggers red uncle recolor):\n");
    printf("Expected: 30 and 70 become black, 50 becomes red\n");
    rb_print(tree.root);
    printf("\n");
}

void test_simple_insert() {
    printf("=== Test: Simple Insert (Black Parent) ===\n");
    printf("Inserting: 10, 5, 15\n");
    printf("Expected: 10 is root (black), 5 and 15 are red children\n\n");

    rb_tree tree = {.root = nullptr};

    rb_insert(&tree, 10);
    printf("After insert 10:\n");
    rb_print(tree.root);
    printf("\n");

    rb_insert(&tree, 5);
    printf("After insert 5:\n");
    rb_print(tree.root);
    printf("\n");

    rb_insert(&tree, 15);
    printf("After insert 15:\n");
    rb_print(tree.root);
    printf("\n");
}

void test_black_uncle_left_left() {
    printf("=== Test: Black Uncle - Left-Left Case ===\n");
    printf("Scenario: Manually built tree with left-left black uncle scenario\n");
    printf("Setup (valid initial tree):\n");
    printf("       50 (B)\n");
    printf("      /  \\\n");
    printf("     30   70 (B)\n");
    printf("    (B)\n");
    printf("    /\n");
    printf("   20 (R)\n\n");
    printf("Then insert 10, which triggers left-left black uncle case\n");
    printf("Expected: Right rotation around 50\n");
    printf("Final: 30 becomes root of subtree, 50 becomes right child, colors swapped\n\n");

    rb_tree tree = {.root = nullptr};

    // Build tree structure
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

    printf("Before insert 10:\n");
    rb_print(tree.root);
    printf("\n");

    printf("Inserting 10 (child of 20):\n");
    printf("Expected: 10(R) with parent 20(R) triggers violation\n");
    printf("  - Uncle of 20 is nil (black)\n");
    printf("  - Left-left case: right rotate around 50\n");
    printf("  - Colors: 30→B, 50→R\n");
    printf("  - Root enforcement: keep root black\n\n");

    rb_insert(&tree, 10);

    printf("After insert 10:\n");
    rb_print(tree.root);
    printf("\nExpected structure:\n");
    printf("       30 (B)     ← Was red, now black (rotated up)\n");
    printf("      /  \\\n");
    printf("     20   50 (R)  ← Was black, now red\n");
    printf("    (B)  /  \\\n");
    printf("    10  nil 70\n");
    printf("   (R)      (B)\n");
    printf("\n");
}

int main() {
    // test_simple_insert();
    // test_red_uncle_case();
    test_black_uncle_left_left();

    return 0;
}
