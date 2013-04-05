#import <stdlib.h>
#import <string.h>
#import <assert.h>
#include "HashTable.h"

#pragma mark PrivateHeader
#define STRING_MAX_LEN 1024
typedef long tindex_t; // Must be signed for error codes
typedef int8_t bool; // Why not?

#pragma mark Table Element Private Header
struct HashTableElement
{
	char *key;
	void *value;
};

static struct HashTableElement *_MakeElement(char *key, void *value);
static void _SetValueInElement(struct HashTableElement *element, void *value);
static void _SetKeyInElement(struct HashTableElement *element, char *key);
static void _FreeElement(struct HashTableElement *element);

#pragma mark Table Element Implementation

static struct HashTableElement *_MakeElement(char *key, void *value)
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

static void _SetValueInElement(struct HashTableElement *element, void *value)
{
	element->value = value;
}

static void _SetKeyInElement(struct HashTableElement *element, char *key)
{
	if (element->key != NULL)
		free(element->key);

	size_t keyLen = strnlen(key, STRING_MAX_LEN);
	element->key = malloc(keyLen * sizeof(char) + 1);
	strncat(element->key, key, keyLen);
}

static void _FreeElement(struct HashTableElement *element)
{
	free(element->key);
	free(element);
}

#pragma mark Hash Table Private Header
struct HashTable
{
	struct HashTableElement **array;
	struct KeyValueList *iteratorKVList;
	struct KeyValueList *collisionsList;
	tindex_t size;
	tindex_t count;
	tindex_t hashLimit;

};

static tindex_t _HashFunction(char *key, tindex_t limit);
static struct HashTable *_AllocateTable(size_t size);
static void _InitTable(struct HashTable *table, size_t size);
// Add Remove
static void _AddKeyValuePair(struct HashTable *table, char *key, void *value);
static void _RemoveKeyValuePair(struct HashTable *table, char *key);
static void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index);
// Checkers
static bool _IsIndexSuitsForKey(struct HashTable *table, tindex_t index, char *key);
static bool _IsElementForKey(struct HashTableElement *element, char *key);
static bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index);
static bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key);
// Setters
static void _SetValueForKey(struct HashTable *table, void *value, char *key);
static void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
static void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
// Getters
static struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index);
// Searching
static tindex_t _FindSuitableIndexForKey(struct HashTable *table, char *key, tindex_t startIndex);
static tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key);

static void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive);

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

static struct HashTable *_AllocateTable(size_t size)
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

	hashTablePointer->iteratorKVList = lst_CreateList();
	hashTablePointer->collisionsList = lst_CreateList();
	return hashTablePointer;
}

static void _InitTable(struct HashTable *table, size_t size)
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
	lst_Free(table->iteratorKVList);
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

	_SetValueForKey(table, value, key);
}

static void _SetValueForKey(struct HashTable *table, void *value, char *key)
{
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

	_RemoveKeyValuePair(table, key);
}

static void _RemoveKeyValuePair(struct HashTable *table, char *key)
{
	tindex_t index = _FindExistingIndexForKey(table, key);
	if (index == -1)
		return;

	_RemoveKeyValuePairAtIndex(table, index);
	lst_RemoveElementWithKey(table->iteratorKVList, key);
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

static tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key)
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

static void _AddKeyValuePair(struct HashTable *table, char *key, void *value)
{
	tindex_t index = _HashFunction(key, table->hashLimit);
	// Do the nice way
	bool suits = _IsIndexSuitsForKey(table, index, key);
	if (suits)
	{
		_SetKeyValuePairAtIndex(table, key, value, index);
	} else
			// Otherwise do collision way
	{
		index = _FindSuitableIndexForKey(table, key, index);
		if (index == -1)
			return;

		_SetCollisionKeyValuePairAtIndex(table, key, value, index);
	}
	// Either way, we must add keyvalue to the iterator KVList
	struct KeyValueList *iteratorKVList = table->iteratorKVList;
	lst_SetValueForKey(iteratorKVList, index, key);
}

static tindex_t _FindSuitableIndexForKey(struct HashTable *table, char *key, tindex_t startIndex)
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

static bool _IsIndexSuitsForKey(struct HashTable *table, tindex_t index, char *key)
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

static bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	return (element == NULL);
}

static bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	if (element == NULL)
		return 0;
	return _IsElementForKey(element, key);
}

static inline struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	return array[index];
}

static bool _IsElementForKey(struct HashTableElement *element, char *key)
{
	char *elementKey = element->key;
	int compareResult = strncmp(elementKey, key, STRING_MAX_LEN);

	return compareResult ? 0 : 1;
}

static void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
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

static void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	struct KeyValueList *collisionList = table->collisionsList;
	lst_SetValueForKey(collisionList, index, key);

	_SetKeyValuePairAtIndex(table, key, value, index);
}

static void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index)
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

#pragma mark Hash
static tindex_t _HashFunction(char *key, tindex_t limit)
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

static inline void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive)
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

#pragma mark Iterator
struct HashTableIteratorInternal
{
	struct KeyValueListIterator *listIterator;
};

static struct HashTableIterator *_AllocateIterator();
static void _InitIterator(struct HashTableIterator *iterator, struct HashTable *table);
static void _InvalidateIterator(struct HashTableIterator *iterator);
static void _IteratorNextFunction(struct HashTableIterator *iterator);

struct HashTableIterator *htbl_IteratorForTable(struct HashTable *table)
{
	if (table == NULL)
		return NULL;
	if (table->count == 0)
		return NULL;

	struct HashTableIterator *iterator = _AllocateIterator();
	_InitIterator(iterator, table);

	return iterator;
}

static struct HashTableIterator *_AllocateIterator()
{
	struct HashTableIterator *iterator = malloc(sizeof(struct HashTableIterator));
	if (iterator == NULL)
	{
		return NULL;
	}

	struct HashTableIteratorInternal *internal = malloc(sizeof(struct HashTableIteratorInternal));
	if (internal == NULL)
	{
		free(iterator);
		return NULL;
	}
	iterator->cheshire = internal;

	return iterator;
}

static void _InitIterator(struct HashTableIterator *iterator, struct HashTable *table)
{
	iterator->table = table;
	struct KeyValueListIterator *listIterator = lst_IteratorForList(table->iteratorKVList); //TODO Error handling?
	iterator->cheshire->listIterator = listIterator;
	iterator->key = listIterator->key;
	iterator->value = table->array[listIterator->value]->value;
	iterator->next = _IteratorNextFunction;
}

static void _IteratorNextFunction(struct HashTableIterator *iterator)
{
	if (htbl_IsValidIterator(iterator) == 0)
		return;

	struct KeyValueList *listInIterator = iterator->cheshire->listIterator->list;
	struct KeyValueList *listInTable = iterator->table->iteratorKVList;
	if (listInIterator != listInTable)
	{
		_InvalidateIterator(iterator);
		return;
	}

	struct KeyValueListIterator *listIterator = iterator->cheshire->listIterator;
	listIterator->next(listIterator);
	if (listIterator->key == NULL)
	{
		_InvalidateIterator(iterator);
		return;
	}

	iterator->key = listIterator->key;
	iterator->value = iterator->table->array[listIterator->value]->value;
}

static void _InvalidateIterator(struct HashTableIterator *iterator)
{
	iterator->key = NULL;
	iterator->value = NULL;
}

int htbl_IsValidIterator(struct HashTableIterator *iterator)
{
	if (iterator == NULL)
		return 0;
	if (iterator->key == NULL)
		return 0;
	return 1;
}

void htbl_FreeIterator(struct HashTableIterator *iterator)
{
	free(iterator->cheshire);
	free(iterator);
}