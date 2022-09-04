#ifndef __LISTNODE_H__
#define __LISTNODE_H__ 
#include"globaltype.h"


typedef struct _list_node {
    struct _list_node* m_prev;
    struct _list_node* m_next; 
} list_node;

typedef struct _list_head {
    list_node m_root;
    int m_size;
} list_head;

#define LIST_ADDR(head) (&(head)->m_root)
#define LIST_FIRST(head) ((head)->m_root.m_next)
#define LIST_LAST(head) ((head)->m_root.m_prev)
#define LIST_SIZE(head) ((head)->m_size)

static inline void INIT_LIST_HEAD(list_head* head) {
    LIST_FIRST(head) = LIST_LAST(head) = LIST_ADDR(head);
    
    LIST_SIZE(head) = 0;
} 

static inline void INIT_LIST_NODE(list_node* h) {
    h->m_prev = h->m_next = h;
}

static inline int list_empty(list_head* head) {
    return LIST_FIRST(head) == LIST_ADDR(head);
}

static inline int node_empty(list_node* node) {
    return node->m_next == node && node->m_prev == node;
}

static inline void __list_add(list_node* node, 
    list_node* prev, list_node* next) {
    node->m_prev = prev;
    node->m_next = next;
    
    prev->m_next = node;
    next->m_prev = node;
}

static inline void list_add_front(list_node* node, list_head* head) {
    __list_add(node, LIST_ADDR(head), LIST_FIRST(head));
    ++LIST_SIZE(head);
}

static inline void list_add_back(list_node* node, list_head* head) {
    __list_add(node, LIST_LAST(head), LIST_ADDR(head));
    ++LIST_SIZE(head);
}

static inline void __list_del(list_node* node) {
    node->m_prev->m_next = node->m_next;
    node->m_next->m_prev = node->m_prev;

    INIT_LIST_NODE(node);
}

static inline void list_del(list_node* node, list_head* head) {
    __list_del(node);
    --LIST_SIZE(head);
}

/* move all of h1 to h2, h1 must not empty */
static inline void list_move(list_head* h1, list_head* h2) {
    if (!list_empty(h1)) {
        LIST_FIRST(h2) = LIST_FIRST(h1);
        LIST_FIRST(h2)->m_prev = LIST_ADDR(h2);
        LIST_LAST(h2) = LIST_LAST(h1);
        LIST_LAST(h2)->m_next = LIST_ADDR(h2);

        LIST_SIZE(h2) += LIST_SIZE(h1); 
        INIT_LIST_HEAD(h1);
    }
}

static void __list_splice(list_head* h, list_node* prev, list_node* next) {
    prev->m_next = LIST_FIRST(h);
    LIST_FIRST(h)->m_prev = prev;
    next->m_prev = LIST_LAST(h);
    LIST_LAST(h)->m_next = next; 
}

/* append all of h1 to head of h2 */
static inline void list_splice_front(list_head* h1, list_head* h2) { 
    if (!list_empty(h1)) {
        __list_splice(h1, LIST_ADDR(h2), LIST_FIRST(h2));

        LIST_SIZE(h2) += LIST_SIZE(h1); 
        INIT_LIST_HEAD(h1);
    }
}

/* append all of h1 to tail of h2 */
static inline void list_splice_back(list_head* h1, list_head* h2) { 
    if (!list_empty(h1)) {
        __list_splice(h1, LIST_LAST(h2), LIST_ADDR(h2));

        LIST_SIZE(h2) += LIST_SIZE(h1); 
        INIT_LIST_HEAD(h1);
    }
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_first_entry(head, type, member) \
	list_entry(LIST_FIRST(head), type, member)

#define list_for_each(pos, head) \
	for (pos = LIST_FIRST(head); pos != LIST_ADDR(head); pos = pos->m_next)

#define list_for_each_prev(pos, head) \
	for (pos = LIST_LAST(head); pos != LIST_ADDR(head); pos = pos->m_prev)
        	
#define list_for_each_safe(pos, n, head) \
	for (pos = LIST_FIRST(head), n = pos->m_next; \
        pos != LIST_ADDR(head); pos = n, n = pos->m_next)

#define list_for_each_prev_safe(pos, n, head) \
	for (pos = LIST_LAST(head), n = pos->m_prev; \
        pos != LIST_ADDR(head); pos = n, n = pos->m_prev)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != LIST_ADDR(head); 	\
	     pos = list_entry(pos->member.m_next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_entry(pos->member.m_next, typeof(*pos), member);	\
	     &pos->member != LIST_ADDR(head); 					\
	     pos = n, n = list_entry(n->member.m_next, typeof(*n), member))
    

typedef struct _hlist_node {
    struct _hlist_node* m_next, **m_pprev;
} hlist_node;
    
typedef struct _hlist_head {
    struct _hlist_node* m_first;
} hlist_head;

#define INIT_HLIST_HEAD(ptr) ((ptr)->m_first = NULL)

static inline void INIT_HLIST_NODE(hlist_node *h) {
    h->m_next = NULL;
	h->m_pprev = NULL;
}

static inline int hlist_unhashed(const hlist_node *h) {
	return !h->m_pprev;
}

static inline int hlist_empty(const hlist_head *h) {
	return !h->m_first;
}

static inline void hlist_del(hlist_node *n) {
	hlist_node *next = n->m_next;
	hlist_node **pprev = n->m_pprev;
    
	*pprev = next;
	if (next) {
		next->m_pprev = pprev;
	}

    INIT_HLIST_NODE(n);
}

static inline void hlist_add(hlist_node *n, hlist_head *h) {
	hlist_node *first = h->m_first;
    
	n->m_next = first;
	if (first) {
		first->m_pprev = &n->m_next;
	}
    
	h->m_first = n;
	n->m_pprev = &h->m_first;
}

static inline void hlist_move(hlist_head *old, hlist_head *x) {
	x->m_first = old->m_first;
    
	if (x->m_first) {
		x->m_first->m_pprev = &x->m_first;
	}
    
	old->m_first = NULL;
}
	
#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->m_first; pos; pos = pos->m_next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->m_first; pos && ({ n = pos->m_next; 1; }); \
	     pos = n)

#define hlist_for_each_entry(tpos, pos, head, member)			 \
	for (pos = (head)->m_first;					 \
	     pos && ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->m_next)

#define hlist_for_each_entry_safe(tpos, pos, n, head, member) 		 \
	for (pos = (head)->m_first;					 \
	     pos && ({ n = pos->m_next; 1; }) && 				 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = n)


typedef struct _order_list_head {
    list_node m_root;
    int m_size;
    int (*m_cmp)(list_node* n1, list_node* n2);
} order_list_head;

#define ORDER_LIST_CMP(head) ((head)->m_cmp)

static inline void INIT_ORDER_LIST_HEAD(
    order_list_head* head,
    int (*cmp)(list_node* n1, list_node* n2)) {
    LIST_FIRST(head) = LIST_LAST(head) = LIST_ADDR(head);
    
    LIST_SIZE(head) = 0;
    ORDER_LIST_CMP(head) = cmp;
}

static inline int order_list_empty(order_list_head* head) {
    return LIST_FIRST(head) == LIST_ADDR(head);
}

static inline void order_list_add(list_node* node, order_list_head* head) {
    int ret = 0;
    list_node* curr = NULL;

    list_for_each_prev(curr, head) {
        ret = ORDER_LIST_CMP(head)(curr, node);
        if (0 >= ret) {
            break;
        }
    }

    __list_add(node, curr, curr->m_next);
    ++LIST_SIZE(head);
}

static inline void order_list_del(list_node* node, order_list_head* head) {
    __list_del(node);
    --LIST_SIZE(head);
}

static inline list_node* order_list_find(list_node* node, order_list_head* head) {
    int ret = 0;
    list_node* pos = NULL;
    
    list_for_each(pos, head) {
        ret = ORDER_LIST_CMP(head)(pos, node);
        if (0 > ret) {
            continue;
        } else if (0 == ret) {
            return pos;
        } else {
            break;
        }
    }

    return NULL;
}
	     
#endif

