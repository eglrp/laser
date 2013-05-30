//-----------------------------------------------------------------------------
//	hd_list.h
//  kevin.ban
//  2007/8/22
//	
//	copy from linux kernel code
//-----------------------------------------------------------------------------


#ifndef _HD_LIST_H_
#define _HD_LIST_H_


/* -----------------------------------------------------------------------*/

/* Doubly linked list implementation to replace the GPL'd one used in
   the Linux kernel. */
//仅仅作标志，无任何具体值
#define MACRO_START
#define MACRO_END

/* TYPES */

typedef struct list_head {
    struct list_head *next;
    struct list_head *prev;
}list_head;

/* MACROS */

#define LIST_HEAD_INIT(name) { &(name), &(name) }

//初始化一个链表头节点，初始时头尾指向同一个节点
#define LIST_HEAD(name) \
        struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD( _list_ )              \
MACRO_START                               \
(_list_)->next = (_list_)->prev = (_list_);   \
MACRO_END

/* FUNCTIONS */

/* Insert an entry _after_ the specified entry */
/* list_add_after() */
#define list_add_after( _newent_, _afterthisent_ )\
do{	\
	struct list_head *newent = _newent_;	\
	struct list_head *afterthisent = _afterthisent_;	\
	struct list_head *next = afterthisent->next;	\
  newent->next = next;	\
  newent->prev = afterthisent;	\
  afterthisent->next = newent;	\
  next->prev = newent;	\
}while(0);

/* Insert an entry _before_ the specified entry */
/* list_add_before() */
#define list_add_before( _newent_, _beforethisent_ )\
do{	\
	struct list_head *newent = _newent_;	\
	struct list_head *beforethisent = _beforethisent_;	\
  struct list_head *prev = beforethisent->prev;	\
  newent->prev = prev;	\
  newent->next = beforethisent;	\
  beforethisent->prev = newent;	\
  prev->next = newent;	\
}while(0);

/* Delete the specified entry */
/* list_del() */
#define list_del( _ent_ )	\
do{	\
	struct list_head *ent = _ent_;	\
	ent->prev->next = ent->next;	\
  ent->next->prev = ent->prev;	\
}while(0);

/* Is this list empty? */
/* list_empty() */
#define list_empty( _list_ )	(_list_.next==&_list_)

/* list_entry - Assuming you have a struct of type _type_ that contains a
   list which has the name _member_ in that struct type, then given the
   address of that list in the struct, _list_, this returns the address
   of the container structure */

#define list_entry( _list_, _type_, _member_ ) \
    ((_type_ *)((char *)(_list_)-(char *)&(((_type_*)0)->_member_)))

/* list_for_each - using _ent_, iterate through list _list_ */

#define list_for_each( _ent_, _list_ )   \
    for ( (_ent_) = (_list_)->next;      \
    (_ent_) != (_list_);                 \
    (_ent_) = (_ent_)->next )

#define list_for_each_reverse( _ent_, _list_ )   \
    for ( (_ent_) = (_list_)->prev;      \
    (_ent_) != (_list_);                 \
    (_ent_) = (_ent_)->prev )

/*
 * list_for_each_entry - this function can be use to iterate over all
 * items in a list* _list_ with it's head at _head_ and link _item_
 */
#define list_for_each_entry(_list_, _head_, _item_)                     \
for ((_list_) = list_entry((_head_)->next, typeof(*_list_), _item_); \
     &((_list_)->_item_) != (_head_);                                 \
     (_list_) = list_entry((_list_)->_item_.next, typeof(*_list_), _item_))

/* -----------------------------------------------------------------------*/
#endif /* #ifndef _HD_LIST_H_ */
/* EOF hd_list.h */
