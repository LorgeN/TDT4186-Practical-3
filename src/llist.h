#ifndef __FLUSH_LIST_H__
#define __FLUSH_LIST_H__

#include <stdbool.h>
#include <stddef.h>

/*
 * Defines a simple linked list structure and methods to modify it
 */

// Internal node structure
struct node_t;

struct list_t {
    struct node_t *head;
    struct node_t *tail;
    size_t size;
};

int llist_append_element(struct list_t *list, void *element);

int llist_remove_element(struct list_t *list, void *element);

int llist_remove_by_flag(struct list_t *list, bool *flags);

void llist_elements(struct list_t *list, void **elements);

void *llist_get(struct list_t *list, size_t index);

#endif