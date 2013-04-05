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

struct HashTableElement *_MakeElement(char *key, void *value);
void _SetValueInElement(struct HashTableElement *element, void *value);
void _SetKeyInElement(struct HashTableElement *element, char *key);
void _FreeElement(struct HashTableElement *element);

#pragma mark Table Element Implementation

struct HashTableElement *_MakeElement(char *key, void *value)
{
	struct HashTableElement *element = calloc(1, sizeof(struct HashTableElement));
	if (element == NULL)
	{
		return NULL;
	}
	_SetKeyInElement(element, key);
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

tindex_t _HashFunction(char *key, tindex_t limit);
struct HashTable *_AllocateTable(size_t size);
void _InitTable(struct HashTable *table, size_t size);
// Add Remove
void _AddKeyValuePair(struct HashTable *table, char *key, void *value);
void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index);
// Checkers
bool _IsIndexSuitsForKey(struct HashTable *table, tindex_t index, char *key);
bool _IsElementForKey(struct HashTableElement *element, char *key);
bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index);
bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key);
// Setters
void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
// Getters
struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index);
// Searching
tindex_t _FindSuitableIndexForKey(struct HashTable *table, char *key, tindex_t startIndex);
tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key);

void _resizeHashTable(struct HashTable *table, tindex_t newSize);
void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive);

#pragma mark Hash Table Implementation

#pragma mark Creation and Decommission
struct HashTable *htbl_Create(size_t capacity)
{
	struct HashTable *hashTable = _AllocateTable(capacity);
	if (hashTable == NULL)
		return NULL;

	_InitTable(hashTable, capacity);
	return hashTable;
}

struct HashTable *_AllocateTable(size_t size)
{
	struct HashTable *hashTablePointer = calloc(1, sizeof(struct HashTable));
	if (hashTablePointer == NULL)
		return NULL;

	hashTablePointer->array = calloc(size, sizeof(struct HashTableElement *));
	if (hashTablePointer->array == NULL)
	{
		free(hashTablePointer);
		return NULL;
	}

	hashTablePointer->fullKeyIndexList = lst_CreateList();
	hashTablePointer->collisionsList = lst_CreateList();
	return hashTablePointer;
}

void _InitTable(struct HashTable *table, size_t size)
{
	table->size = size;
	table->hashLimit = size;
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
void htbl_SetValueForKey(struct HashTable *table, void *value, char *key)
{
	if (table == NULL)
		return;
	if (key == NULL)
		return;
	if (strlen(key) == 0)
		return;
	if (value == NULL)
		return;

	tindex_t index = _FindExistingIndexForKey(table, key);
	if (index != -1)
	{
		_SetKeyValuePairAtIndex(table, key, value, index);
		return;
	}

	_AddKeyValuePair(table, key, value);
}

void htbl_RemoveKey(struct HashTable *table, char *key)
{
	if (table == NULL)
		return;
	if (key == NULL)
		return;
	if (strlen(key) == 0)
		return;

	tindex_t index = _FindExistingIndexForKey(table, key);
	if (index == -1)
		return;

	_RemoveKeyValuePairAtIndex(table, index);
}

void *htbl_ValueForKey(struct HashTable *table, char *key)
{
	if (table == NULL)
		return NULL;
	if (key == NULL)
		return NULL;
	if (strlen(key) == 0)
		return NULL;

	tindex_t index = _FindExistingIndexForKey(table, key);
	if (index < 0)
		return NULL;

	struct HashTableElement *element = _ElementAtIndex(table, index);
	return element->value;
}

tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key)
{
	// Search in hash index
	tindex_t desiredIndex = _HashFunction(key, table->hashLimit);
	bool containsDesiredElement = _IsElementAtIndexForKey(table, desiredIndex, key);
	if (containsDesiredElement)
	{
		return desiredIndex;
	}
	// Search in collisions
	struct KeyValueList *collisionList = table->collisionsList;
	tindex_t collisionIndex = lst_ValueForKey(collisionList, key); // Will return -1 if not found
	return collisionIndex;
}

void _AddKeyValuePair(struct HashTable *table, char *key, void *value)
{
	tindex_t desiredIndex = _HashFunction(key, table->hashLimit);
	// Do the nice way
	bool suits = _IsIndexSuitsForKey(table, desiredIndex, key);
	if (suits)
	{
		_SetKeyValuePairAtIndex(table, key, value, desiredIndex);
		return;
	}
	// Otherwise do collision way
	tindex_t collisionIndex = _FindSuitableIndexForKey(table, key, desiredIndex);
	if (collisionIndex != -1)
	{
		_SetCollisionKeyValuePairAtIndex(table, key, value, collisionIndex);
		return;
	}
}

tindex_t _FindSuitableIndexForKey(struct HashTable *table, char *key, tindex_t startIndex)
{
	tindex_t currentIndex = startIndex;

	while (_IsIndexSuitsForKey(table, currentIndex, key) == 0)
	{
		_loopIncrement(&currentIndex, 1, table->size);
		if (currentIndex == startIndex) /* If made a full circle */
			return -1;
	}

	return currentIndex;
}

bool _IsIndexSuitsForKey(struct HashTable *table, tindex_t index, char *key)
{
	if (_IsElementAtIndexEmpty(table, index) == 1)
	{
		return 1;
	}
	if (_IsElementAtIndexForKey(table, index, key) == 1)
	{
		return 1;
	}
	return 0;
}

bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	return (element == NULL);
}

bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	if (element == NULL)
		return 0;
	return _IsElementForKey(element, key);
}

inline struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	return array[index];
}

bool _IsElementForKey(struct HashTableElement *element, char *key)
{
	char *elementKey = element->key;
	int compareResult = strncmp(elementKey, key, STRING_MAX_LEN);

	return compareResult ? 0 : 1;
}

void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	if (element == NULL)
	{
		element = _MakeElement(key, value);
		struct HashTableElement **array = table->array;
		array[index] = element;
		table->count++;
	} else
	{
		_SetValueInElement(element, value);
	}
}

void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	struct KeyValueList *collisionList = table->collisionsList;
	lst_SetValueForKey(collisionList, index, key);

	_SetKeyValuePairAtIndex(table, key, value, index);
}

void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	//TODO Check collisions!
	_FreeElement(array[index]);
	array[index] = NULL;

	table->count--;
}

size_t htbl_TableSize(struct HashTable *table)
{
	if (table == NULL)
		return 0;
	return (size_t) table->size;
}

void _resizeHashTable(struct HashTable *table, tindex_t newSize)
{

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
	tindex_t inc = abs((int) increment);

	if (value + inc >= maxValueExclusive)
	{
		*variable = (value + inc) % maxValueExclusive;
	} else
	{
		*variable = value + inc;
	}
}

