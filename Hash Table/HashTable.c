#import <stdlib.h>
#import <string.h>
#import <assert.h>
#include "HashTable.h"
#include "KeyValueList.h"

#pragma mark PrivateHeader
#define STRING_MAX_LEN 1024
typedef long tindex_t; // Must be signed for error codes
typedef int8_t bool; // Why not?

#pragma mark Table Element Private Header
struct HashTableElement
{
	char *key;
	char *value;
};

struct HashTableElement * _MakeElement(char *key, void *value);

void _SetValueInElement(struct HashTableElement *element, void *value);
void _SetKeyInElement(struct HashTableElement *element, char *key);
void _FreeElement(struct HashTableElement *element);

#pragma mark Table Element Implementation

struct HashTableElement * _MakeElement(char *key, void *value)
{
	struct HashTableElement *element = calloc(1,sizeof(struct HashTableElement));
	if (element == NULL)
	{
		return NULL;
	}
	_SetKeyInElement(element,key);
	_SetValueInElement(element, value);
	return element;
}

void _SetValueInElement(struct HashTableElement *element, void *value)
{
	element->value = value;
}

void _SetKeyInElement(struct HashTableElement *element, char *key)
{
	if (element->key != NULL)
		free(element->key);

	size_t keyLen = strnlen(key, STRING_MAX_LEN);
	element->key = malloc(keyLen * sizeof(char) + 1);
	strncat(element->key, key, keyLen);
}

void _FreeElement(struct HashTableElement *element)
{
	free(element->key);
	free(element);
}

#pragma mark Hash Table Private Header
struct HashTable
{
	struct HashTableElement **array;
	struct KeyValueList *fullKeyIndexList;
	struct KeyValueList *collisionsList;
	tindex_t size;
	tindex_t count;
	tindex_t hashLimit;

};

tindex_t _HashTableInitialSize();
tindex_t _HashFunction(char *key, tindex_t limit);
struct HashTable *_AllocateTable();
void _InitTable(struct HashTable *table);
struct HashTableElement * _ElementAtIndex(struct HashTable *table, tindex_t index);
bool _IsElementForKey(struct HashTableElement *element,char *key);
bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index);
void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive);
bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key);
void _setKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
tindex_t _FindIndexForKey(struct HashTable *table, char *key, bool returnFirstEmpty);
void _removeKeyValuePairAtIndex(struct HashTable *table, tindex_t index);
void _resizeHashTable(struct HashTable *table,tindex_t newSize);

#pragma mark Hash Table Implementation

#pragma mark Creation and Decommission
struct HashTable *htbl_Create()
{
	struct HashTable *hashTable = _AllocateTable();
	if (hashTable == NULL)
		return NULL;

 	_InitTable(hashTable);
	return hashTable;
}

struct HashTable *_AllocateTable()
{
	struct HashTable *hashTablePointer = calloc(1, sizeof(struct HashTable));
	if (hashTablePointer == NULL)
		return NULL;

	hashTablePointer->array = calloc((size_t) _HashTableInitialSize(), sizeof(struct HashTableElement *));
	if (hashTablePointer->array == NULL)
	{
		free(hashTablePointer);
		return NULL;
	}

	hashTablePointer->fullKeyIndexList = lst_CreateList();
	hashTablePointer->collisionsList = lst_CreateList();
	return hashTablePointer;
}

void _InitTable(struct HashTable *table)
{
	table->size = _HashTableInitialSize();
	table->hashLimit = _HashTableInitialSize();
}

tindex_t _HashTableInitialSize()
{
	return 7; // I like number 7.
}

void htbl_Free(struct HashTable *table)
{
	if (table == NULL)
		return;
	free(table->array);
	//TODO Iterate to free objects
	lst_Free(table->fullKeyIndexList);
	lst_Free(table->collisionsList);
	free(table);
}

#pragma mark Adding and Removing
void htbl_SetValueForKey(struct HashTable *table, void* value, char *key)
{
	if (table == NULL)
		return;
	if (key == NULL)
		return;
	if (strlen(key) == 0)
		return;
	if (value == NULL)
		return;

	if (table->size == table->count)
		_resizeHashTable(table, table->count+1);

	bool returnEmptyIndex = 1;

	tindex_t index = _FindIndexForKey(table, key, returnEmptyIndex);
	if (index < 0)
		return;

	_setKeyValuePairAtIndex(table, key, value, index);
}

void htbl_RemoveKey(struct HashTable *table, char *key)
{
	if (table == NULL)
		return;
	if (key == NULL)
		return;
	if (strlen(key) == 0)
		return;

	bool returnsEmpty = 0;
	tindex_t index = _FindIndexForKey(table, key, returnsEmpty);
	if (index < 0)
		return;

	_removeKeyValuePairAtIndex(table, index);
}

void* htbl_ValueForKey(struct HashTable *table, char *key)
{
	if (table == NULL)
		return NULL;
	if (key == NULL)
		return NULL;
	if (strlen(key) == 0)
		return NULL;

	bool returnEmptyIndex = 0;
	tindex_t index = _FindIndexForKey(table, key, returnEmptyIndex);
	if (index < 0)
		return NULL;

	struct HashTableElement *element = _ElementAtIndex(table, index);
	return element->value;
}


/* Returns first empty index if not found*/
tindex_t _FindIndexForKey(struct HashTable *table, char *key, bool returnFirstEmpty)
{

	tindex_t hashIndex = _HashFunction(key,table->hashLimit);
	tindex_t currentIndex = hashIndex;

	while(_IsElementAtIndexEmpty(table, currentIndex) == 0)
	{
		if (_IsElementAtIndexForKey(table, currentIndex, key))
			return currentIndex;

		_loopIncrement(&currentIndex, 1, table->size);
		if (currentIndex == hashIndex) /* If made a full circle */
			return -1;
	}

	return returnFirstEmpty ? currentIndex : -1;
}

bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index)
{
	struct HashTableElement * element = _ElementAtIndex(table, index);
	return (element == NULL);
}

bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key)
{
	struct HashTableElement * element = _ElementAtIndex(table, index);
	return _IsElementForKey(element, key);
}

struct HashTableElement * _ElementAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	return array[index];
}

bool _IsElementForKey(struct HashTableElement *element,char *key)
{
	char *elementKey = element->key;
	int compareResult = strncmp(elementKey, key, STRING_MAX_LEN);

	return compareResult ? 0 : 1;
}

void _setKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	if (element == NULL)
	{
		element = _MakeElement(key, value);
		struct HashTableElement **array = table->array;
		array[index] = element;
		table->count++ ;
	} else {
		_SetValueInElement(element, value);
	}
}

void _removeKeyValuePairAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	_FreeElement(array[index]);
	array[index] = NULL;
	table->count--;
}

void _resizeHashTable(struct HashTable *table,tindex_t newSize)
{
	struct HashTableElement ** newArray = realloc(table->array,sizeof(struct HashTableElement *) * newSize );
	if (newArray == NULL)
		return;

	if (table->size < newSize)
		memset(newArray+table->size, 0, sizeof(struct HashTableElement *)*(newSize - table->size));

	table->array = newArray;
	table->size = newSize;
}

#pragma mark Hash
tindex_t _HashFunction(char *key, tindex_t limit)
{
	if (key == NULL)
		return -1;

	size_t keyLength = strnlen(key, STRING_MAX_LEN);
	if (keyLength == 0)
		return -1;

	tindex_t hash = 0;
	for (size_t i = 0; i < keyLength; ++i)
		_loopIncrement(&hash, key[i], limit);

	assert(hash >= 0);
	assert(hash < limit);
	return hash;
}

inline void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive)
{
	tindex_t value = *variable;
	tindex_t inc = abs((int)increment);

	if (value+inc >= maxValueExclusive)
	{
		*variable = (value+inc) % maxValueExclusive ;
	}else{
		*variable = value+inc;
	}
}

