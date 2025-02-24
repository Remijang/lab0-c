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
    if (!el->value) {
        free(el);
        return false;
    }
    strncpy(el->value, s, len + 1);

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
    if (!el->value) {
        free(el);
        return false;
    }
    strncpy(el->value, s, len + 1);

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
    if (!head || list_empty(head))
        return false;
    struct list_head *left, *left_safe;
    list_for_each_safe (left, left_safe, head) {
        element_t *el_left = list_entry(left, element_t, list);
        bool has = false;
        struct list_head *right, *right_safe;
        for (right = el_left->list.next, right_safe = right->next;
             right != head; right = right_safe, right_safe = right_safe->next) {
            element_t *el_right = list_entry(right, element_t, list);
            if (!strcmp(el_left->value, el_right->value)) {
                has = true;
                list_del(&el_right->list);
                free(el_right->value);
                free(el_right);
            }
        }
        left_safe = left->next;
        if (has) {
            list_del(left);
            free(el_left->value);
            free(el_left);
        }
    }
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

#define list_swap_t(x, y)           \
    do {                            \
        struct list_head *SWAP = x; \
        x = y;                      \
        y = SWAP;                   \
    } while (0)
/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *li, *li_safe;
    list_for_each_safe (li, li_safe, head) {
        list_swap_t(li->prev, li->next);
    }
    list_swap_t(head->prev, head->next);
    return;
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // TODO: raise the exception of the case k <= 0
    if (!head || list_empty(head))
        return;
    struct list_head *li, *li_safe;
    struct list_head *cur = head, *next = cur->next;
    for (int i = 0; i < k; ++i, next = next->next) {
        if (next == head)
            return;
    }
    while (true) {
        for (li = cur->next, li_safe = li->next; li != next;
             li = li_safe, li_safe = li_safe->next)
            list_swap_t(li->prev, li->next);
        list_swap_t(cur->next->next, next->prev->prev);
        list_swap_t(cur->next, next->prev);
        cur = next->prev;
        for (int i = 0; i < k; ++i, next = next->next)
            if (next == head)
                return;
    }
}

/* ensure that head, left, right are non-NULL pointer */
void merge2sorted(struct list_head *head,
                  struct list_head *left,
                  struct list_head *right,
                  bool descend)
{
    while (!list_empty(left) && !list_empty(right)) {
        const element_t *el1 = list_entry(left->next, element_t, list);
        const element_t *el2 = list_entry(right->next, element_t, list);
        int c = strcmp(el1->value, el2->value);
        struct list_head *li = descend ? (c < 0 ? left->next : right->next)
                                       : (c > 0 ? right->next : left->next);
        list_del(li);
        list_add_tail(li, head);
    }
    list_splice_tail(left, head);
    list_splice_tail(right, head);
    return;
}

void merge_sort(struct list_head *head, bool descend)
{
    /* handle the basic case */
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    /* use slow-fast pointer to find the mid */
    struct list_head *slow = head->next, *fast = head->next->next;
    while (fast != head && fast != head->prev) {
        slow = slow->next;
        fast = fast->next->next;
    }
    /* break into two circular list and one empty head */
    struct list_head *left = head->next, *right = slow->next;
    struct list_head *left_end = slow, *right_end = head->prev;
    struct list_head dummy_l = {left_end, left};
    struct list_head dummy_r = {right_end, right};
    left->prev = left_end->next = &dummy_l;
    right->prev = right_end->next = &dummy_r;
    INIT_LIST_HEAD(head);
    /* solve the subproblem */
    merge_sort(&dummy_l, descend);
    merge_sort(&dummy_r, descend);
    /* combine the result */
    merge2sorted(head, &dummy_l, &dummy_r, descend);
    return;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    return merge_sort(head, descend);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return q_size(head);
    element_t *el = list_entry(head->prev, element_t, list);
    element_t *el_safe = list_entry(el->list.prev, element_t, list);
    const char *s = el->value;
    int len = 0;
    for (; &el->list != head;
         el = el_safe,
         el_safe = list_entry(el_safe->list.prev, element_t, list)) {
        if (strcmp(el->value, s) > 0) {
            list_del(&el->list);
            free(el->value);
            free(el);
        } else
            s = el->value, ++len;
    }
    return len;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return q_size(head);
    element_t *el = list_entry(head->prev, element_t, list);
    element_t *el_safe = list_entry(el->list.prev, element_t, list);
    const char *s = el->value;
    int len = 0;
    for (; &el->list != head;
         el = el_safe,
         el_safe = list_entry(el_safe->list.prev, element_t, list)) {
        if (strcmp(el->value, s) < 0) {
            list_del(&el->list);
            free(el->value);
            free(el);
        } else
            s = el->value, ++len;
    }
    return len;
}

void merge2queue(struct list_head *left, struct list_head *right, bool descend)
{
    queue_contex_t *qu_l = list_entry(left, queue_contex_t, chain);
    queue_contex_t *qu_r = list_entry(right, queue_contex_t, chain);
    /* merge list to the dummy head */
    LIST_HEAD(dummy);
    merge2sorted(&dummy, qu_l->q, qu_r->q, descend);
    /* replace the dummy head with the head of the left queue */
    dummy.next->prev = qu_l->q;
    dummy.prev->next = qu_l->q;
    qu_l->q->next = dummy.next;
    qu_l->q->prev = dummy.prev;
    /* make the queue be empty */
    INIT_LIST_HEAD(qu_r->q);
    return;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    /* handle the basic case */
    if (!head || list_empty(head))
        return 0;
    /* naive approach to merge queue */
    struct list_head *li = head->next->next;
    while (li != head) {
        merge2queue(head->next, li, descend);
        li = li->next;
    }
    queue_contex_t *qu = list_entry(head->next, queue_contex_t, chain);
    return q_size(qu->q);
}
