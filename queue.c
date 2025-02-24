#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/* Create an empty queue */
struct list_head *q_new()
{
    /* allocate the memory for the head of the queue */
    element_t *el_new = malloc(sizeof(element_t));
    if (!el_new)
        return NULL;

    /* initialize the head of the queue */
    *el_new =
        (element_t){.value = NULL,
                    .list = (struct list_head){&el_new->list, &el_new->list}};
    return &el_new->list;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    /* traverse the queue and release the memory */
    element_t *el = NULL, *el_safe;
    list_for_each_entry_safe (el, el_safe, head, list) {
        list_del(&el->list);
        free(el->value);
        free(el);
    }
    /* now "el" is the head element */
    if (el->value)
        free(el->value);
    free(el);
    return;
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head || !s)
        return false;

    /* allocate the memory for the new element */
    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return false;

    /* initialize the new element */
    int len = strlen(s);
    el->value = malloc(sizeof(char) * (len + 1));
    if (!el->value)
        return false;
    strncpy(el->value, s, len + 1);
    INIT_LIST_HEAD(&el->list);

    /* insert the new element at the head of the queue */
    list_add(&el->list, head);

    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head || !s)
        return false;

    /* allocate the memory for the new element */
    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return false;

    /* initialize the new element */
    int len = strlen(s);
    el->value = malloc(sizeof(char) * (len + 1));
    if (!el->value)
        return false;
    strncpy(el->value, s, len + 1);
    INIT_LIST_HEAD(&el->list);

    /* insert the new element at the tail of the queue */
    list_add_tail(&el->list, head);

    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    /* extract the element from the head of the queue */
    element_t *el = list_entry(head->next, element_t, list);
    list_del(head->next);

    /* copy the content to sp if sp is non-NULL */
    if (sp) {
        strncpy(sp, el->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    return el;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    /* extract the element from the tail of the queue */
    element_t *el = list_entry(head->prev, element_t, list);
    list_del(head->prev);

    /* copy the content to sp if sp is non-NULL */
    if (sp) {
        strncpy(sp, el->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    return el;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    int len = 0;
    struct list_head *li;
    list_for_each (li, head)
        ++len;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *slow = head->next, *fast = head->next->next;
    while (fast != head && fast != head->prev) {
        slow = slow->next;
        fast = fast->next->next;
    }
    list_del_init(slow);
    q_free(slow);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head **li;
    for (li = &head->next; *li != head && *li != head->prev;
         li = &(*li)->next->next) {
        struct list_head *first = *li, *second = (*li)->next;
        second->next->prev = first;
        *li = second;

        first->next = second->next;
        second->prev = first->prev;

        first->prev = second;
        second->next = first;
    }
    return;
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head) {}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend) {}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    return 0;
}
