#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"
#include "hashtable.h"

struct hashtable
{
    int length;
    linkedlist_t **buckets;
};

typedef struct hashtable hashtable_t;

/**
 * Hash function to hash a key into the range [0, max_range)
 */
static int hash(int key, int max_range)
{
    key = (key > 0) ? key : -key;
    return key % max_range;
}

hashtable_t *ht_init(int num_buckets)
{
    struct hashtable *new = malloc(sizeof(hashtable_t));
    new->length = num_buckets;
    new->buckets = malloc(sizeof(linkedlist_t*) * num_buckets);
    for(int i = 0; i < num_buckets; ++i)
    {
        new->buckets[i] = ll_init();
    }
    return new;
}

void ht_free(hashtable_t *table)
{
    for (int i = 0; i < table->length; ++i)
    {
        ll_free(table->buckets[hash(i, table->length)]);
    }
    free(table->buckets);    
    free(table);
}

void ht_add(hashtable_t *table, int key, int value)
{
    // If the key already exists, replace the value.
    ll_add(table->buckets[hash(key, table->length)], key, value);
}

int ht_get(hashtable_t *table, int key)
{
    // If it does not exist, return 0
    return ll_get(table->buckets[hash(key, table->length)], key);
}

int ht_size(hashtable_t *table)
{
    int ret = 0;
    for (int i = 0; i < table->length; ++i)
    {
        ret = ret + ll_size(table->buckets[i]);
    }
    return ret;
}
