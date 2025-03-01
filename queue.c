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
    struct list_head *li_new = malloc(sizeof(struct list_head));
    if (!li_new)
        return NULL;
    INIT_LIST_HEAD(li_new);
    return li_new;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    /* traverse the queue and release the memory */
    element_t *el = NULL, *el_safe;
    list_for_each_entry_safe (el, el_safe, head, list) {
        q_release_element(el);
    }

    free(head);
    return;
}

inline element_t *q_new_element(char *s)
{
    /* allocate the memory for the new element */
    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return NULL;

    /* initialize the new element */
    el->value = strdup(s);
    if (!el->value) {
        free(el);
        return NULL;
    }

    return el;
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    /* generate new element and handle exceptions */
    element_t *el;
    if (!head || !s || !(el = q_new_element(s)))
        return false;

    /* insert the new element at the head of the queue */
    list_add(&el->list, head);

    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    /* generate new element and handle exceptions */
    element_t *el;
    if (!head || !s || !(el = q_new_element(s)))
        return false;

    /* insert the new element at the tail of the queue */
    list_add_tail(&el->list, head);

    return true;
}

inline void q_copy_string(char *dst, char *src, size_t bufsize)
{
    /* copy the content to dst if dst is non-NULL */
    if (dst) {
        strncpy(dst, src, bufsize - 1);
        dst[bufsize - 1] = '\0';
    }
    return;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    /* extract the element from the head of the queue */
    element_t *el = list_first_entry(head, element_t, list);
    list_del(head->next);

    q_copy_string(sp, el->value, bufsize);

    return el;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    /* extract the element from the tail of the queue */
    element_t *el = list_last_entry(head, element_t, list);
    list_del(head->prev);

    q_copy_string(sp, el->value, bufsize);

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
    list_del(slow);
    element_t *el = list_entry(slow, element_t, list);
    q_release_element(el);
    return true;
}

#define list_for_each_safe_partial(node, safe, start, end)       \
    for (node = (start)->next, safe = node->next; node != (end); \
         node = safe, safe = node->next)

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
        list_for_each_safe_partial(right, right_safe, left, head)
        {
            element_t *el_right = list_entry(right, element_t, list);
            if (!strcmp(el_left->value, el_right->value)) {
                has = true;
                list_del(&el_right->list);
                q_release_element(el_right);
            }
        }
        /* it is necessary to reassign the value of safe var */
        left_safe = left->next;
        if (has) {
            list_del(left);
            q_release_element(el_left);
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    q_reverseK(head, 2);
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
    if (!head || list_empty(head) || k == 1)
        return;
    struct list_head *li;
    LIST_HEAD(cache);
    LIST_HEAD(dummy_head);
    bool check = true;
    while (check) {
        for (int _ = 0; _ < k; ++_) {
            if (list_empty(head)) {
                check = false;
                break;
            }
            li = head->next;
            list_del(li);
            list_add(li, &cache);
        }
        if (!check)
            q_reverse(&cache);
        list_splice_tail(&cache, &dummy_head);
        INIT_LIST_HEAD(&cache);
    }
    list_add(head, &dummy_head);
    list_del(&dummy_head);
}

/* ensure that head, left, right are non-NULL pointer */
void merge2sorted(struct list_head *head,
                  struct list_head *left,
                  struct list_head *right,
                  bool descend)
{
    while (!list_empty(left) && !list_empty(right)) {
        const element_t *el1 = list_first_entry(left, element_t, list);
        const element_t *el2 = list_first_entry(right, element_t, list);
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
    struct list_head dummy_l, dummy_r;
    list_cut_position(&dummy_l, head, slow);
    list_cut_position(&dummy_r, head, head->prev);
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

#define list_for_each_entry_safe_reverse(entry, safe, head, member)    \
    for (entry = list_entry((head)->prev, typeof(*entry), member),     \
        safe = list_entry(entry->member.prev, typeof(*entry), member); \
         &entry->member != (head); entry = safe,                       \
        safe = list_entry(safe->member.prev, typeof(*entry), member))

int q_strict(struct list_head *head, int descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return q_size(head);
    element_t *el = NULL, *el_safe;
    const char dummy[1] = "", *s = dummy;
    int len = 0;
    list_for_each_entry_safe_reverse(el, el_safe, head, list)
    {
        if (strcmp(el->value, s) * descend > 0) {
            list_del(&el->list);
            q_release_element(el);
        } else
            s = el->value, ++len;
    }
    return len;
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    return q_strict(head, 1);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    return q_strict(head, -1);
}

void merge2queue(struct list_head *left, struct list_head *right, bool descend)
{
    queue_contex_t *qu_l = list_entry(left, queue_contex_t, chain);
    queue_contex_t *qu_r = list_entry(right, queue_contex_t, chain);
    /* merge list to the dummy head */
    LIST_HEAD(dummy);
    merge2sorted(&dummy, qu_l->q, qu_r->q, descend);
    /* replace the dummy head with the head of the left queue */
    list_add(qu_l->q, &dummy);
    list_del(&dummy);
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
    queue_contex_t *qu = list_first_entry(head, queue_contex_t, chain);
    return q_size(qu->q);
}
