#ifndef __LIST_H
#define __LIST_H
#include <stddef.h>

#ifndef true
#define true 1
#endif
	
#ifndef false
#define false 0
#endif

#undef offsetof
#define offsetof(s, m)   (unsigned long)&(((s *) 0)->m)

#define containerof(ptr, type, member) \
    ((type *)((unsigned long)(ptr) - offsetof(type, member)))

struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
	struct list_node name = LIST_HEAD_INIT(name)

static inline void list_initialize(struct list_node *list)
{
    list->prev = list->next = list;
}

static inline void list_add_head(struct list_node *list, struct list_node *item)
{
    item->next = list->next;
    item->prev = list;
    list->next->prev = item;
    list->next = item;
}

static inline void list_add_tail(struct list_node *list, struct list_node *item)
{
    item->prev = list->prev;
    item->next = list;
    list->prev->next = item;
    list->prev = item;
}

static inline void list_delete(struct list_node *item)
{
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = item->next = 0;
}

static inline struct list_node *list_next(struct list_node *list, struct list_node *item)
{
    if (item->next != list)
        return item->next;
    else
        return (struct list_node *)0;
}

/* iterates over the list, node should be struct list_node* */
#define list_for_every(list, node) \
    for(node = (list)->next; node != (list); node = node->next)

/* iterates over the list, entry should be the container structure type* */
#define list_for_each_entry(list, entry, type, member) \
    for((entry) = containerof((list)->next, type, member);\
        &(entry)->member != (list);\
        (entry) = containerof((entry)->member.next, type, member))

static inline int list_is_empty(struct list_node *list)
{
    return (list->next == list) ? 1 : 0;
}

static inline size_t list_length(struct list_node *list)
{
    size_t cnt = 0;
    struct list_node *node = list;
    list_for_every(list, node)
    {
        cnt++;
    }

    return cnt;
}

#endif
