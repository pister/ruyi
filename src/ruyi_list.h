//
//  ruyi_ds.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_list_h
#define ruyi_list_h

#include "ruyi_basics.h"
#include "ruyi_value.h"

/**
 * Double direct linked_list
 */
struct tag_ruyi_list_item {
    struct tag_ruyi_list_item *prev;
    struct tag_ruyi_list_item *next;
    ruyi_value value;
};

typedef struct tag_ruyi_list_item ruyi_list_item;

/**
 * Double direct linked_list
 */
typedef struct {
    ruyi_list_item *first;
    ruyi_list_item *last;
} ruyi_list;

/**
 * Create a double linked list
 */
ruyi_list* ruyi_list_create(void);

/**
 * Destroy the double linked list,
 * NOTICE: this will NOT free the item when it is an pointer,
 * so you may free these pointers before ruyi_list_destroy as youself knows detail.
 * params:
 * list - the list to be destroy
 */
void ruyi_list_destroy(ruyi_list* list);

/**
 * Add the item on the Last:
 * params:
 * list - the target list
 * value - item value
 */
void ruyi_list_add_last(ruyi_list* list, ruyi_value value);

/**
 * Add the item on the First:
 * params:
 * list - the target list
 * value - item value
 */
void ruyi_list_add_first(ruyi_list* list, ruyi_value value);

/**
 * Remove the first item
 * params:
 * list - the target list
 * ret_value - the removed item value, this param can be NULL
 * return:
 * TRUE remove success, FALSE remova fail or list is empty
 */
BOOL ruyi_list_remove_first(ruyi_list* list, ruyi_value *ret_value);

/**
 * Remove the last item
 * params:
 * list - the target list
 * ret_value - the removed item value, this param can be NULL
 * return:
 * TRUE remove success, FALSE remova fail or list is empty
 */
BOOL ruyi_list_remove_last(ruyi_list* list, ruyi_value *ret_value);

/**
 * Get the first item
 * params:
 * list - the target list
 * ret_value - the return item value
 * return:
 * TRUE get success, FALSE if list is empty
 */
BOOL ruyi_list_get_first(const ruyi_list* list, ruyi_value *ret_value);

/**
 * Get the last item
 * params:
 * list - the target list
 * ret_value - the return item value
 * return:
 * TRUE get success, FALSE if list is empty
 */
BOOL ruyi_list_get_last(const ruyi_list* list, ruyi_value *ret_value);

/**
 * Get the list is empty
 * params:
 * list - the target list
 * return:
 * TRUE when empty, FALSE is not empty
 */
BOOL ruyi_list_empty(const ruyi_list* list);

/**
 * Insert after the item
 * NOTICE: if the list not contains base_item, this operator ocurr undefined things.
 * params:
 * list - the target list
 * base_item - the base item
 * value - target value
 */
void ruyi_list_insert_after(ruyi_list* list, ruyi_list_item* base_item, ruyi_value value);

/**
 * Insert before the item
 * NOTICE: if the list not contains base_item, this operator ocurr undefined things.
 * params:
 * list - the target list
 * base_item - the base item
 * value - target value
 */
void ruyi_list_insert_before(ruyi_list* list, ruyi_list_item* base_item, ruyi_value value);

/**
 * Remove the the from list
 * NOTICE: if the list not contains item, this operator ocurr undefined things.
 * params:
 * list - the target list
 * item - the item tobe removing
 */
void ruyi_list_remove_item(ruyi_list* list, ruyi_list_item* item);

/**
 * Find the first item which value is equals input value
 * params:
 * list - the target list
 * value - tobe found value
 * return:
 * the item pointer when found, NULL when not found
 */
ruyi_list_item* ruyi_list_find_first(const ruyi_list* list, ruyi_value value);

/**
 * Find the last item which value is equals input value
 * params:
 * list - the target list
 * value - tobe found value
 * return:
 * the item pointer when found, NULL when not found
 */
ruyi_list_item* ruyi_list_find_last(const ruyi_list* list, ruyi_value value);

#endif /* ruyi_list_h */
