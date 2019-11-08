//
//  ruyi_hashtable.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/16.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_hashtable.h"
#include <stdio.h>
#include <string.h> // for memset
#include "ruyi_mem.h"

#define HASHTABLE_LOAD_FACTOR 0.75
#define HASHTABLE_DEFAULT_INIT_CAP 16
#define HASHTABLE_MIN_CAP 4

struct ruyi_hash_entry {
    ruyi_value key;
    ruyi_value value;
    struct ruyi_hash_entry *next;
    UINT32 hash;
};

static struct ruyi_hash_entry* ruyi_create_hash_entry(UINT32 hash, ruyi_value key, ruyi_value value, struct ruyi_hash_entry *next) {
    struct ruyi_hash_entry *entry = (struct ruyi_hash_entry *)ruyi_mem_alloc(sizeof(struct ruyi_hash_entry));
    entry->hash = hash;
    entry->key = key;
    entry->value = value;
    entry->next = next;
    return entry;
}

ruyi_hashtable * ruyi_hashtable_create_with_init_cap(UINT32 init_cap) {
    ruyi_hashtable *hashtable = ruyi_mem_alloc(sizeof(ruyi_hashtable));
    if (init_cap < HASHTABLE_MIN_CAP) {
        init_cap = HASHTABLE_MIN_CAP;
    }
    hashtable->capacity = init_cap;
    hashtable->length = 0;
    hashtable->table = ruyi_mem_alloc(init_cap * sizeof(struct ruyi_hash_entry*));
    memset(hashtable->table, 0, init_cap * sizeof(struct ruyi_hash_entry*));
    return hashtable;
}

ruyi_hashtable * ruyi_hashtable_create(void) {
    return ruyi_hashtable_create_with_init_cap(HASHTABLE_DEFAULT_INIT_CAP);
}

void ruyi_hashtable_destroy(ruyi_hashtable *hashtable) {
    assert(hashtable);
    UINT32 i;
    struct ruyi_hash_entry *next_entry;
    struct ruyi_hash_entry *current_entry;
    if (hashtable->table) {
        for (i = 0; i < hashtable->capacity; i++) {
            next_entry = hashtable->table[i];
            while (next_entry) {
                current_entry = next_entry;
                next_entry = next_entry->next;
                ruyi_mem_free(current_entry);
            }
        }
        ruyi_mem_free(hashtable->table);
    }
    ruyi_mem_free(hashtable);
}

static void rehash(ruyi_hashtable *hashtable) {
    UINT32 old_capacity = hashtable->capacity;
    UINT32 new_capacity = (old_capacity << 1) + 1;
    UINT32 i, index;
    struct ruyi_hash_entry *old_entry, *entry;
    struct ruyi_hash_entry **old_table = hashtable->table;
    struct ruyi_hash_entry **new_table = ruyi_mem_alloc(new_capacity * sizeof(struct ruyi_hash_entry*));
    memset(new_table, 0, new_capacity * sizeof(struct ruyi_hash_entry*));
    hashtable->table = new_table;
    hashtable->capacity = new_capacity;
    for (i = old_capacity ; i-- > 0 ;) {
        for (old_entry = old_table[i] ; old_entry != NULL; ) {
            entry = old_entry;
            old_entry = old_entry->next;
            index = entry->hash % new_capacity;
            entry->next = new_table[index];
            new_table[index] = entry;
        }
    }
    // free old_table, but not free entry
    if (old_table) {
        ruyi_mem_free(old_table);
    }
}


static void add_entry(ruyi_hashtable *hashtable, UINT32 hash, ruyi_value key, ruyi_value value, UINT32 index) {
    struct ruyi_hash_entry **tab;
    if (hashtable->length >= (UINT32)(HASHTABLE_LOAD_FACTOR * hashtable->capacity)) {
        rehash(hashtable);
        hash = ruyi_value_hashcode(key);
        index = hash % hashtable->capacity;
    }
    tab = hashtable->table;
    tab[index] = ruyi_create_hash_entry(hash, key, value, tab[index]);
    hashtable->length++;
}

void ruyi_hashtable_put(ruyi_hashtable *hashtable, ruyi_value key, ruyi_value value) {
    assert(hashtable);
    UINT32 hash = ruyi_value_hashcode(key);
    UINT32 index = hash % hashtable->capacity;
    struct ruyi_hash_entry *entry;
    entry = hashtable->table[index];
    for(; entry != NULL ; entry = entry->next) {
        if ((entry->hash == hash) && ruyi_value_equals(entry->key, key)) {
            entry->value = value;
            return;
        }
    }
    add_entry(hashtable, hash, key, value, index);
}

BOOL ruyi_hashtable_get(const ruyi_hashtable *hashtable, ruyi_value key, ruyi_value *ret_value) {
    assert(hashtable);
    struct ruyi_hash_entry **tab = hashtable->table;
    UINT32 hash = ruyi_value_hashcode(key);
    UINT32 index = hash % hashtable->capacity;
    struct ruyi_hash_entry *entry = tab[index];
    for (; entry != NULL ; entry = entry->next) {
        if ((entry->hash == hash) && ruyi_value_equals(entry->key, key)) {
            if (ret_value) {
                *ret_value = entry->value;
            }
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ruyi_hashtable_delete(ruyi_hashtable *hashtable, ruyi_value key) {
    assert(hashtable);
    struct ruyi_hash_entry **tab = hashtable->table;
    UINT32 hash = ruyi_value_hashcode(key);
    UINT32 index = hash % hashtable->capacity;
    struct ruyi_hash_entry *entry = tab[index];
    struct ruyi_hash_entry *prev = NULL;
    while (entry) {
        if ((entry->hash == hash) && ruyi_value_equals(entry->key, key)) {
            if (prev != NULL) {
                prev->next = entry->next;
            } else {
                tab[index] = entry->next;
            }
            hashtable->length --;
            // old_value = entry->value;
            ruyi_mem_free(entry);
            return TRUE;
        }
        prev = entry;
        entry = entry->next;
    }
    return FALSE;
}

void ruyi_hashtable_clear(ruyi_hashtable *hashtable) {
    assert(hashtable);
    struct ruyi_hash_entry **tab = hashtable->table;
    struct ruyi_hash_entry *entry, *curr_entry;
    UINT32 i;
    for (i = 0; i < hashtable->capacity; i++) {
        entry = tab[i];
        while (entry) {
            curr_entry = entry;
            entry = entry->next;
            ruyi_mem_free(curr_entry);
        }
        tab[i] = NULL;
    }
    hashtable->length = 0;
}

UINT32 ruyi_hashtable_length(const ruyi_hashtable *hashtable) {
    assert(hashtable);
    return hashtable->length;
}

void ruyi_hashtable_iterator_get(const ruyi_hashtable *hashtable, ruyi_hashtable_iterator *ret_iterator) {
    assert(hashtable);
    assert(ret_iterator);
    ret_iterator->hashtable = hashtable;
    ret_iterator->index = 0;
    if (hashtable->capacity > 0) {
        ret_iterator->entry = hashtable->table[0];
    } else {
        ret_iterator->entry = NULL;
    }
}

BOOL ruyi_hashtable_iterator_next(ruyi_hashtable_iterator *iterator, ruyi_value *ret_key, ruyi_value *ret_value) {
    const struct ruyi_hash_entry *entry;
    while (TRUE) {
        if (iterator->entry) {
            entry = iterator->entry;
            if (ret_key) {
                *ret_key = entry->key;
            }
            if (ret_value) {
                *ret_value = entry->value;
            }
            iterator->entry = iterator->entry->next;
            return TRUE;
        } else {
            iterator->index++;
            if (iterator->index >= iterator->hashtable->capacity) {
                return FALSE;
            }
            iterator->entry = iterator->hashtable->table[iterator->index];
        }
    }
    return FALSE;
}
