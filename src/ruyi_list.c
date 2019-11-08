//
//  ruyi_list.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_list.h"
#include "ruyi_mem.h"


ruyi_list* ruyi_list_create(void) {
    ruyi_list * list = ruyi_mem_alloc(sizeof(ruyi_list));
    list->first = NULL;
    list->last = NULL;
    return list;
}

void ruyi_list_destroy(ruyi_list* list) {
    assert(list);
    ruyi_list_item* next = list->first;
    ruyi_list_item* item;    
    while (next) {
        item = next;
        next = item->next;
        ruyi_mem_free(item);
    }
    ruyi_mem_free(list);
}

void ruyi_list_add_last(ruyi_list* list, ruyi_value value) {
    assert(list);
    ruyi_list_item *old_last = list->last;
    ruyi_list_item *new_last = ruyi_mem_alloc(sizeof(ruyi_list_item));
    new_last->value = value;
    new_last->next = NULL;
    new_last->prev = old_last;
    list->last = new_last;
    if (old_last) {
        old_last->next = new_last;
    } else {
        list->first = new_last;
    }
}

void ruyi_list_add_first(ruyi_list* list, ruyi_value value) {
    assert(list);
    ruyi_list_item *old_first = list->first;
    ruyi_list_item *new_first = ruyi_mem_alloc(sizeof(ruyi_list_item));
    new_first->value = value;
   // old_first->prev = NULL;
    new_first->next = old_first;
    list->first = new_first;
    if (old_first) {
        old_first->prev = new_first;
    } else {
        list->last = new_first;
    }
}

BOOL ruyi_list_remove_first(ruyi_list* list, ruyi_value *ret_value) {
    assert(list);
    ruyi_list_item *curr_first = list->first;
    if (!curr_first) {
        return FALSE;
    }
    list->first = curr_first->next;
    if (curr_first->next) {
        curr_first->next->prev = NULL;
    }
    if (list->first == NULL) {
        list->last = NULL;
    }
    if (ret_value) {
        *ret_value = curr_first->value;
    }
    ruyi_mem_free(curr_first);
    return TRUE;
}

BOOL ruyi_list_remove_last(ruyi_list* list, ruyi_value *ret_value) {
    assert(list);
    ruyi_list_item *curr_last = list->last;
    if (!curr_last) {
        return FALSE;
    }
    list->last = curr_last->prev;
    if (curr_last->prev) {
        curr_last->prev->next = NULL;
    }
    if (list->last == NULL) {
        list->first = NULL;
    }
    if (ret_value) {
        *ret_value = curr_last->value;
    }
    ruyi_mem_free(curr_last);
    return TRUE;
}

BOOL ruyi_list_get_first(const ruyi_list* list, ruyi_value *ret_value) {
    assert(list);
    assert(ret_value);
    ruyi_list_item *curr_first = list->first;
    if (!curr_first) {
        return FALSE;
    }
    *ret_value = curr_first->value;
    return TRUE;
}

BOOL ruyi_list_get_last(const ruyi_list* list, ruyi_value *ret_value) {
    assert(list);
    assert(ret_value);
    ruyi_list_item *curr_last = list->last;
    if (!curr_last) {
        return FALSE;
    }
    *ret_value = curr_last->value;
    return TRUE;
}

BOOL ruyi_list_empty(const ruyi_list* list) {
    assert(list);
    return NULL == list->first;
}

void ruyi_list_insert_after(ruyi_list* list, ruyi_list_item* base_item, ruyi_value value) {
    assert(list);
    assert(base_item);
    ruyi_list_item *curr_next = base_item->next;
    ruyi_list_item *new_next = ruyi_mem_alloc(sizeof(ruyi_list_item));
    new_next->value = value;
    new_next->next = curr_next;
    new_next->prev = new_next;
    base_item->next = new_next;
    if (curr_next) {
        curr_next->prev = new_next;
    } else {
        list->last = new_next;
    }
}

void ruyi_list_insert_before(ruyi_list* list, ruyi_list_item* base_item, ruyi_value value) {
    assert(list);
    assert(base_item);
    ruyi_list_item *curr_prev = base_item->prev;
    ruyi_list_item *new_prev = ruyi_mem_alloc(sizeof(ruyi_list_item));
    new_prev->value = value;
    new_prev->next = base_item;
    new_prev->prev = curr_prev;
    base_item->prev = new_prev;
    if (curr_prev) {
        curr_prev->next = new_prev;
    } else {
        list->first = new_prev;
    }
}


void ruyi_list_remove_item(ruyi_list* list, ruyi_list_item* item) {
    assert(list);
    ruyi_list_item* prev;
    ruyi_list_item* next;
    if (!item) {
        return;
    }
    prev = item->prev;
    next = item->next;
    if (prev) {
        prev->next = next;
    } else {
        list->first = next;
    }
    if (next) {
        next->prev = prev;
    } else {
        list->last = prev;
    }
    ruyi_mem_free(item);
}

ruyi_list_item* ruyi_list_find_first(const ruyi_list* list, ruyi_value value) {
    assert(list);
    ruyi_list_item* item = list->first;
    while (item) {
        if (ruyi_value_equals(item->value, value)) {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

ruyi_list_item* ruyi_list_find_last(const ruyi_list* list, ruyi_value value) {
    assert(list);
    ruyi_list_item* item = list->last;
    while (item) {
        if (ruyi_value_equals(item->value, value)) {
            return item;
        }
        item = item->prev;
    }
    return NULL;
}
