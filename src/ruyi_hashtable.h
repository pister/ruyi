//
//  ruyi_hashtable.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/16.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_hashtable_h
#define ruyi_hashtable_h

#include "ruyi_basics.h"
#include "ruyi_value.h"

struct ruyi_hash_entry;

typedef struct {
    struct ruyi_hash_entry **table;
    UINT32 capacity;
    UINT32 length;
} ruyi_hashtable;

typedef struct {
    struct ruyi_hash_entry *entry;
    UINT32 index;
    ruyi_hashtable *hashtable;
} ruyi_hashtable_iterator;

/**
 * Create a hashtable with give init capacity
 * params:
 * init_cap - the init capacity
 */
ruyi_hashtable * ruyi_hashtable_create_with_init_cap(UINT32 init_cap);

/**
 * Create a hashtable with default init capacity
 */
ruyi_hashtable * ruyi_hashtable_create(void);

/**
 * Destory the hashtable
 * NOTICE: this will NOT free the item when it is an pointer,
 * so you may free these pointers before ruyi_hashtable_destory as youself knows detail.
 */
void ruyi_hashtable_destory(ruyi_hashtable *hashtable);

/**
 * Get the hashtable items' count
 * params:
 * hashtable - the target hashtable
 * return:
 * the hashtable length
 */
UINT32 ruyi_hashtable_length(ruyi_hashtable *hashtable);

/**
 * Put the key-value to the hashtable, if key exists, it will replace old value
 * params:
 * hashtable - the target hashtable
 * key - the input key
 * value - the input value
 */
void ruyi_hashtable_put(ruyi_hashtable *hashtable, ruyi_value key, ruyi_value value);

/**
 * Get the value from hashtable by key,
 * if ret_value is NULL, just return the key has exist in the hashtable
 * params:
 * hashtable - the target hashtable
 * key - key to be find
 * ret_value - the found value, if it exists, this param can be NULL
 * return:
 * TRUE - indicate the key has found, FALSE indicate not found.
 */
BOOL ruyi_hashtable_get(ruyi_hashtable *hashtable, ruyi_value key, ruyi_value *ret_value);

/**
 * Delete the entry by key
 * params:
 * hashtable - the target hashtable
 * key - key to be delete
 * return:
 * TRUE - indicate the delete success, FALSE indicate not found by key.
 */
BOOL ruyi_hashtable_delete(ruyi_hashtable *hashtable, ruyi_value key);

/**
 * Clear whole hashtable
 * params:
 * hashtable - the target hashtable
 */
void ruyi_hashtable_clear(ruyi_hashtable *hashtable);

/**
 * Get the iterator for the hashtable, it used to iterate the hashtable
 * params:
 * hashtable - the target hashtable
 * ret_iterator - the return iterator
 */
void ruyi_hashtable_iterator_get(ruyi_hashtable *hashtable, ruyi_hashtable_iterator *ret_iterator);

/**
 * Get the current iterate value and turn to next, if it returns FALSE, it indicates iterator is finished.
 * it usually used like this:
 * ruyi_value key, value;
 * ruyi_hashtable_iterator it;
 * ruyi_hashtable_iterator_get(table, &it);
 * while (ruyi_hashtable_iterator_next(&it, &key, &value)) {
 *     // todo something for key and value
 * }
 *
 * params:
 * iterator - the target iterator
 * ret_key - the return key, it call be NULL
 * ret_value - the return value, it call be NULL
 * return:
 * TRUE indicates has next, or FALSE iterate finished.
 */
BOOL ruyi_hashtable_iterator_next(ruyi_hashtable_iterator *iterator, ruyi_value *ret_key, ruyi_value *ret_value);

#endif /* ruyi_hashtable_h */
