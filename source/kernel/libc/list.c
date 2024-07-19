// list.c - Simple list implementation, shouldn't be in libc but whatever

#include "include/libc/list.h" // Main header file

// list_destroy(list_t *list) - Destroys the contents of a list
void list_destroy(list_t *list) {
    // Free all of the contents of a list by iterating through them
    node_t *node = list->head;
    while (node) {
        kfree(node->value);
        node = node->next;
    }
}

// list_free(list_t *list) - Frees the structure of the list
void list_free(list_t *list) {
    // Same thing, basically
    node_t *node = list->head;
    while (node) {
        node_t *s = node->next;
        kfree(node);
        node = s;
    }
}

// list_append(list_t *list, node_t *node) - Append something to the list
void list_append(list_t *list, node_t *node) {
    node->next = NULL;

    // Insert it onto the end of the list
    if (!list->tail) {
        list->head = node;
        node->prev = NULL;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
    }
    list->tail = node;
    list->length++;
}


// list_insert(list_t *list, void *item) - Insert an item into the list
void list_insert(list_t *list, void *item) {
    node_t *node = kmalloc(sizeof(node_t));
    node->value = item;
    node->next = NULL;
    node->prev = NULL;
    list_append(list, node);
}


// list_append_after(list_t *list, node_t *before, node_t *node) - Append after a node
void list_append_after(list_t *list, node_t *before, node_t *node) {
    if (!list->tail) {
        list_append(list, node);
        return;
    } 

    if (before == NULL) {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
        list->length++;
        return;
    }

    if (before == list->tail) {
        list->tail = node;
    } else {
        before->next->prev = node;
        node->next = before->next;
    }

    node->prev = before;
    before->next = node;
    list->length++;
}

// list_insert_after(list_t *list, node_t *before, void *item) - Insert after a node
void list_insert_after(list_t *list, node_t *before, void *item) {
    node_t *node = kmalloc(sizeof(node_t));
    node->value = item;
    node->next = NULL;
    node->prev = NULL;
    list_append_after(list, before, node);
}

// list_create() - Create a new list
list_t *list_create() {
    list_t *out = kmalloc(sizeof(list_t));
    out->head = NULL;
    out->tail = NULL;
    out->length = 0;
    return out;
}

// list_find(list_t *list, void *value) - Find a node in a list
node_t *list_find(list_t *list, void *value) {
    foreach(item, list) {
        if (item->value == value) return item;
    }

    return NULL;
}

// list_delete(list_t *list, node_t *node) - Remove a node from the list
void list_delete(list_t *list, node_t *node) {
    if (node == list->head) list->head = node->next;
    if (node == list->tail) list->tail = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    node->prev = NULL;
    node->next = NULL;
    list->length--;
}

// list_remove(list_t *list, size_t index) - Remove index from list
void list_remove(list_t *list, size_t index) {
    if (index > list->length) return;
    size_t i = 0;
    node_t *node = list->head;
    while (i < index) {
        node = node->next;
        i++;
    }

    list_delete(list, node);
}

// list_pop(list_t *list) - Remove and return the last value in the list
node_t *list_pop(list_t *list) {
    if (!list->tail) return NULL;
    node_t *out = list->tail;
    list_delete(list, list->tail);
    return out;
}

// list_dequeue(list_t *list) - Remove and return the first value in the list
node_t *list_dequeue(list_t *list) {
    if (!list->head) return NULL; // how?
    node_t *out = list->head;
    list_delete(list, list->head);
    return out;
}

// list_copy(list_t *original) - Createas a copy of the original
list_t *list_copy(list_t *original) {
    list_t *out = list_create();
    node_t *node = original->head;
    while (node) list_insert(out, node->value);
    return out;
}

// list_merge(list_t *target, list_t *source) - Merges a list destructively
void list_merge(list_t *target, list_t *source) {
    if (target->tail) {
        target->tail->next = source->head;
    } else {
        target->head = source->head;
    }

    if (source->tail) target->tail = source->tail;
    target->length += source->length;
    kfree(source);
}
