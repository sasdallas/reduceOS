// tree.c - General purpose tree implementation, shouldn't even be in libc.

#include "include/libc/tree.h" // Main header file

// tree_create() - Create a new tree
tree_t *tree_create() {
    tree_t *tree = kmalloc(sizeof(tree_t));
    tree->nodes = 0;
    tree->root = 0;
    return tree;
}

// tree_set_root(tree_t *tree, void *value) - Set the root node of the tree
void tree_set_root(tree_t *tree, void *value) {
    tree_node_t *root = tree_node_create(value);
    tree->root = root;
    tree->nodes = 1;
}

// tree_node_destroy(tree_node_t *node) - Frees the contents of a node and its children (however, it doesn't do the node itself)
void tree_node_destroy(tree_node_t *node) {
    foreach(child, node->children) {
        tree_node_destroy((tree_node_t*)child->value);
    }

    kfree(node->value);
}

// tree_destroy(tree_t *tree) - Frees the contents of a tree, but not the nodes
void tree_destroy(tree_t *tree) {
    if (tree->root) tree_node_destroy(tree->root);
}

// tree_node_free(tree_node_t *node) - Frees a node and its children, but not their contents
void tree_node_free(tree_node_t *node) {
    if (!node) return;
    foreach(child, node->children) {
        tree_node_free(child->value);
    }

    kfree(node);
} 

// tree_free(tree_t *tree) - Frees all of the nodes in a tree, but not their contents
void tree_free(tree_t *tree) {
    tree_node_free(tree->root);
}

// tree_node_create(void *value) - Creates a new tree node pointing to the given value
tree_node_t *tree_node_create(void *value) {
    tree_node_t *out = kmalloc(sizeof(tree_node_t));
    out->value = value;
    out->children = list_create();
    out->parent = NULL;
    return out;
}

// tree_node_insert_child_node(tree_t *tree, tree_node_t *parent, tree_node_t *node) - Insert a node as a child of a parent
void tree_node_insert_child_node(tree_t *tree, tree_node_t *parent, tree_node_t *node) {
    list_insert(parent->children, node);
    node->parent = parent;
    tree->nodes++;
}


// tree_node_insert_child(tree_t *tree, tree_node_t *parent, void *value) - Insert a fresh node as a child of a parnet
tree_node_t *tree_node_insert_child(tree_t *tree, tree_node_t *parent, void *value) {
    tree_node_t *out = tree_node_create(value);
    tree_node_insert_child_node(tree, parent, out);
    return out;
} 

// tree_node_find_parent(tree_node_t *haystack, tree_node_t *needle) - Recursive node part of tree_find_parent
tree_node_t *tree_node_find_parent(tree_node_t *haystack, tree_node_t *needle) {
    tree_node_t *found = NULL;
    foreach(child, haystack->children) {
        if (child->value == needle) return haystack;
        found = tree_node_find_parent((tree_node_t*)child->value, needle);
        if (found) break;
    }

    return found;
} 

// tree_find_parent(tree_t *tree, tree_node_t *node) - Return the parent of a node
tree_node_t *tree_find_parent(tree_t *tree, tree_node_t *node) {
    if (!tree->root) return NULL;
    return tree_node_find_parent(tree->root, node);
}

// tree_count_children(tree_node_t *node) - Counts the children in a node
size_t tree_count_children(tree_node_t *node) {
    if (!node || !node->children) return 0;
    size_t out = node->children->length;
    foreach(child, node->children) {
        out += tree_count_children((tree_node_t*)child->value);
    }
    return out;
}

// tree_node_parent_remove(tree_t *tree, tree_node_t *parent, tree_node_t *node) - Remove a node when parent i sknown
void tree_node_parent_remove(tree_t *tree, tree_node_t *parent, tree_node_t *node) {
    tree->nodes -= tree_count_children(node) + 1;
    list_delete(parent->children, list_find(parent->children, node));
    tree_node_free(node);
}

// tree_node_remove(tree_t *tree, tree_node_t *node) - Remove an entire branch given its root
void tree_node_remove(tree_t *tree, tree_node_t *node) {
    tree_node_t *parent = node->parent;
    if (!parent) {
        if (node == tree->root) {
            tree->nodes = 0;
            tree->root = NULL;
            tree_node_free(node);
        }
    }

    tree_node_parent_remove(tree, parent, node);
}

// tree_remove(tree_t *tree, tree_node_t *node) - Remove this node and move its children onto its parent's list of children
void tree_remove(tree_t *tree, tree_node_t *node) {
    tree_node_t *parent = node->parent;

    if (!parent) return;
    tree->nodes--;
    list_delete(parent->children, list_find(parent->children, node));
    foreach(child, node->children) {
        ((tree_node_t*)child->value)->parent = parent;
    }


    list_merge(parent->children, node->children);
    kfree(node);
}

// tree_break_off(tree_t *tree, tree_node_t *node) - Remove node from tree
void tree_break_off(tree_t *tree, tree_node_t *node) {
    tree_node_t *parent = node->parent;
    if (!parent) return;
    list_delete(parent->children, list_find(parent->children, node));
}

// tree_node_find(tree_node_t *node, void *search, tree_comparator_t comparator) - Find a node in the tree
tree_node_t *tree_node_find(tree_node_t *node, void *search, tree_comparator_t comparator) {
    if (comparator(node->value, search)) return node;

    tree_node_t *found;
    foreach(child, node->children) {
        found = tree_node_find((tree_node_t*)child->value, search, comparator);
        if (found) return found;
    }
    return NULL;
}

// tree_find(tree_t *tree, void *value, tree_comparator_t comparator) - Find a value within a tree
tree_node_t *tree_find(tree_t *tree, void *value, tree_comparator_t comparator) {
    return tree_node_find(tree->root, value, comparator);
}