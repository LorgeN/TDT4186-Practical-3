#ifndef __FLUSH_LIST_H__
#define __FLUSH_LIST_H__

#include <stdbool.h>
#include <stddef.h>

/*
 * Defines a simple linked list structure and methods to modify it
 */

// Internal node structure
struct node_t;

// Basic doubly linked list structure. Should be initialized with
// nullpointers for head/tail and size of 0.
struct list_t {
    struct node_t *head;
    struct node_t *tail;
    size_t size;
};

/**
 * @brief Append the given element
 *
 * @param list The list
 * @param element The element
 * @return int - 0 if success, non-zero otherwise
 */
int llist_append_element(struct list_t *list, void *element);

/**
 * @brief Remove the given element
 *
 * @param list The list
 * @param element The element
 * @return int - 0 if success, non-zero otherwise
 */
int llist_remove_element(struct list_t *list, void *element);

/**
 * @brief Remove the flagged elements from the list
 *
 * @param list The list
 * @param flags An array of boolean values of the same size as the list,
 * where true means remove, false means keep
 * @return int - 0 if success, non-zero otherwise
 */
int llist_remove_by_flag(struct list_t *list, bool *flags);

/**
 * @brief Copies pointers to all elements into the pointer "elements"
 *
 * @param list The list of elements
 * @param elements The space in memory allocated for output
 */
void llist_elements(struct list_t *list, void **elements);

/**
 * @brief Get the element at a specific index
 *
 * @param list The list
 * @param index The index
 * @return void* - The element at the specified index
 */
void *llist_get(struct list_t *list, size_t index);

#endif