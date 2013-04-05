#ifndef HashTable_h
#define HashTable_h

struct HashTable;

struct HashTable *htbl_Create(size_t capacity);

void htbl_SetValueForKey(struct HashTable *table, void *value, char *key);

void *htbl_ValueForKey(struct HashTable *table, char *key);

void htbl_RemoveKey(struct HashTable *table, char *key);

size_t htbl_TableSize(struct HashTable *table);

void htbl_Free(struct HashTable *table);

#endif