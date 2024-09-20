// list.h - Just a small list implementation (shouldn't be in libc, but I don't care)
// You can find this impl. here: https://github.com/stevej/osdev/blob/master/kernel/include/list.h (FORKED FROM TOARUOS)


#ifndef LIST_H
#define LIST_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/stddef.h> // size_t
#include <kernel/mem.h> // Memory functions

// Typedefs
typedef struct node { 
    struct node *next; // Next node
    struct node *prev; // Previous node
    void *value; // Value of this node
    void *owner; // Owner of the node
} node_t;


typedef struct list {
    struct node *head;
    struct node *tail;
    size_t length;
} list_t;

// Macros
#define foreach(i, list) for (node_t *i = list->head; i != NULL; i = i->next)

// Functions
void list_destroy(list_t * list);
void list_free(list_t * list);
void list_append(list_t * list, node_t * item);
void list_insert(list_t * list, void * item);
list_t * list_create();
node_t * list_find(list_t * list, void * value);
void list_remove(list_t * list, size_t index);
void list_delete(list_t * list, node_t * node);
node_t * list_pop(list_t * list);
node_t * list_dequeue(list_t * list);
list_t * list_copy(list_t * original);
void list_merge(list_t * target, list_t * source);

void list_append_after(list_t * list, node_t * before, node_t * node);
void list_insert_after(list_t * list, node_t * before, void * item);
void list_insert_before(list_t * list, node_t * after, void * item);

#endif
