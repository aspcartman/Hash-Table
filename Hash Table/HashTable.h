#ifndef HashTable_h
#define HashTable_h

#include "KeyValueList.h"

struct HashTable;
struct HashTableIteratorInternal;

struct HashTableIterator
{
	char *key;
	void *value;

	void (*next)(struct HashTableIterator *);

	struct HashTable *table;
	struct HashTableIteratorInternal *cheshire;
};
struct HashTable *htbl_Create(size_t capacity);

void htbl_SetValueForKey(struct HashTable *table, void *value, char *key);
void *htbl_ValueForKey(struct HashTable *table, char *key);
void htbl_RemoveKey(struct HashTable *table, char *key);

size_t htbl_TableSize(struct HashTable *table);
size_t htbl_Count(struct HashTable *table);

struct HashTableIterator *htbl_IteratorForTable(struct HashTable *table);
int htbl_IsValidIterator(struct HashTableIterator *iterator);
void htbl_FreeIterator(struct HashTableIterator *iterator);

void htbl_Free(struct HashTable *table);

#endif