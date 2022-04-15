#include "llist.h"

#include <stdlib.h>
#include <unistd.h>

// Doubly linked list
struct node_t {
    struct node_t *next;
    struct node_t *prev;
    void *element;
};

static struct node_t *find_node(struct list_t *list, void *element) {
    struct node_t *current = list->head;
    if (current == NULL) {
        return NULL;
    }

    while (current != NULL && current->element != element) {
        current = current->next;
    }

    return current;
}

int llist_append_element(struct list_t *list, void *element) {
    struct node_t *node = malloc(sizeof(struct node_t));
    if (node == NULL) {
        return 1;
    }

    node->element = element;
    node->prev = list->tail;
    node->next = NULL;

    list->size++;

    // Check if this is a new list
    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
        return 0;
    }

    list->tail->next = node;
    list->tail = node;
    return 0;
}

int llist_remove_element(struct list_t *list, void *element) {
    struct node_t *node = find_node(list, element);
    if (node == NULL) {
        return 1;  // Can't remove it if it isn't in the list
    }

    // Check if this is the head
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        // If it is the head, the next element should be the new head
        list->head = node->next;
    }

    // Check if this is the tail
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        // If it is the tail, the previous node is now the new tail
        list->tail = node->prev;
    }

    list->size--;

    // We allocated this when it was added, so we need to free it here
    free(node);
    return 0;
}

void llist_elements(struct list_t *list, void **elements) {
    // This method assumes that elements points to an array that
    // has space for all our required pointers
    struct node_t *current = list->head;
    size_t index = 0;
    while (current != NULL) {
        elements[index++] = current->element;
        current = current->next;
    }
}

void *llist_get(struct list_t *list, size_t index) {
    struct node_t *current = list->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }

    return current->element;
}