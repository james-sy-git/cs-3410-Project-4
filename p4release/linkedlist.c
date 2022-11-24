#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

struct linkedlist
{
    struct linkedlist_node *first;
    int length;
};

struct linkedlist_node
{
    int key;
    int value;
    struct linkedlist_node *next;
};
typedef struct linkedlist_node linkedlist_node_t;

linkedlist_t *ll_init()
{
    linkedlist_t *list = malloc(sizeof(linkedlist_t));
    list->length = 0;
    list->first = NULL;
    return list;
}

void ll_free(linkedlist_t *list)
{
    linkedlist_node_t *iternode = list->first;
    while (iternode != NULL)
    {
        linkedlist_node_t *temp = iternode->next;
        free(iternode);
        iternode = temp;
    }
    free(list);
}

void ll_add(linkedlist_t *list, int key, int value)
{
    linkedlist_node_t *iternode = list->first;
    while (iternode != NULL)
    {
        if (iternode->key == key) 
        {
            iternode->value = value;
            return;
        }        
        iternode = iternode->next;
    }
    linkedlist_node_t *newnode = malloc(sizeof(linkedlist_node_t));
    newnode->key = key;
    newnode->value = value;
    newnode->next = list->first;
    list->first = newnode;
    list->length = list->length + 1;
}

int ll_get(linkedlist_t *list, int key)
{
    linkedlist_node_t *iternode = list->first;
    while (iternode != NULL)
    {
        if (iternode->key == key)
        {
            return iternode->value;
        }
        iternode = iternode->next;
    }
    return 0;
}

int ll_size(linkedlist_t *list)
{
    return list->length;
}
