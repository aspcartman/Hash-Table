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
	if (element->key == NULL)
	{
		free(element);
		return NULL;
	}

	_SetValueInElement(element, value); /* Value is not being copied */

	return element;
}

static void _SetKeyInElement(struct HashTableElement *element, char *key)
{
	/* Does not frees the previous element key,
	 * because it's not intended to be used on already
	  * initialized element */
	assert(element->key == NULL);

	size_t keyLen = strnlen(key, STRING_MAX_LEN);
	assert(keyLen != 0);
	element->key = malloc((keyLen + 1) * sizeof(char));
	if (element->key == NULL)
		return;
	memcpy(element->key, key, keyLen + 1);
	assert(strncmp(element->key, key, keyLen) == 0);
}

static void _SetValueInElement(struct HashTableElement *element, void *value)
{
	element->value = value;
}

static void _FreeElement(struct HashTableElement *element)
{
	free(element->key);
	free(element);
}

#pragma mark Hash Table
struct HashTable
{
	struct HashTableElement **array;
	struct KeyValueList *iteratorKVList;
	struct KeyValueList *collisionsList;
	tindex_t size;
	tindex_t count;
	tindex_t hashLimit;

};

// Creation
static struct HashTable *_AllocateTable(size_t size);
static struct HashTable *_InitTable(struct HashTable *table, size_t size);
// Destruction
static void _FreeTableContentsButLeaveStruct(struct HashTable *table);
static void _FreeTableStructButLeakContents(struct HashTable *table);
// Add
static void _AddKeyValuePair(struct HashTable *table, char *key, void *value);
// Remove
static void _RemoveKeyValuePair(struct HashTable *table, char *key);
static void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index);
bool _RemoveAllElementsUsingIterator(struct HashTable *table);
void _RemoveAllElementsByHand(struct HashTable *table);
// Setters
static void _SetValueForKey(struct HashTable *table, void *value, char *key);
static void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
static void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index);
// Getters
static struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index);
// Checkers
static bool _IsElementForKey(struct HashTableElement *element, char *key);
static bool _IsElementAtIndexEmpty(struct HashTable *table, tindex_t index);
static bool _IsElementAtIndexForKey(struct HashTable *table, tindex_t index, char *key);
// Searching
static tindex_t _FindEmptyIndexAfterIndex(struct HashTable *table, tindex_t startIndex);
static tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key);
// Optimization
static void _OptimizeTable(struct HashTable *table);
static void _ResizeTable(struct HashTable *table, size_t newSize);
// Stuff
static void _loopIncrement(tindex_t *variable, tindex_t increment, tindex_t maxValueExclusive);
tindex_t _HashFunction(char *key, tindex_t limit);

#pragma mark Creation
struct HashTable *htbl_Create(size_t capacity)
{
	struct HashTable *hashTable = _AllocateTable(capacity);
	if (hashTable == NULL)
		return NULL;

	hashTable = _InitTable(hashTable, capacity);
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

	return hashTablePointer;
}

static struct HashTable *_InitTable(struct HashTable *table, size_t size)
{
	// How to beautify this section?
	table->iteratorKVList = lst_CreateList();
	if (table->iteratorKVList == NULL)
	{
		free(table->array);
		free(table);
		return NULL;
	}

	table->collisionsList = lst_CreateList();
	if (table->collisionsList == NULL)
	{
		free(table->iteratorKVList);
		free(table->array);
		free(table);
		return NULL;
	}

	table->size = size;
	table->hashLimit = size;

	return table;
}

#pragma mark Destruction
void htbl_Free(struct HashTable *table)
{
	if (table == NULL)
		return;

	_FreeTableContentsButLeaveStruct(table);
	_FreeTableStructButLeakContents(table);
}

static void _FreeTableContentsButLeaveStruct(struct HashTable *table)
{
	bool succeed = _RemoveAllElementsUsingIterator(table);
	if (succeed == 0)
		_RemoveAllElementsByHand(table);

	lst_Free(table->iteratorKVList);
	lst_Free(table->collisionsList);
	free(table->array);
}

bool _RemoveAllElementsUsingIterator(struct HashTable *table)
{
	struct HashTableIterator *iterator = htbl_IteratorForTable(table);
	if (iterator == NULL)
		return 0;
	while (htbl_IsValidIterator(iterator))
	{
		htbl_RemoveKey(table, iterator->key);
		iterator->next(iterator);
	}
	htbl_FreeIterator(iterator);
	assert(table->count == 0);
	return 1;
}

void _RemoveAllElementsByHand(struct HashTable *table)
{
	for (int i = 0; i < table->size; ++i)
	{
		if (table->array[i] == NULL)
			continue;

		_RemoveKeyValuePairAtIndex(table, i);
	}
}

static void _FreeTableStructButLeakContents(struct HashTable *table)
{
	free(table);
}

#pragma mark Adding
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

	size_t len = strlen(key);
	char backedUpKey[len + 1];
	memcpy(backedUpKey, key, len + 1);

	_OptimizeTable(table);
	_SetValueForKey(table, value, backedUpKey);
}

static void _AddKeyValuePair(struct HashTable *table, char *key, void *value)
{
	tindex_t index = _HashFunction(key, table->hashLimit);
	// Do the nice way
	bool empty = _IsElementAtIndexEmpty(table, index);
	if (empty)
	{
		_SetKeyValuePairAtIndex(table, key, value, index);
	} else
			// Otherwise do collision way
	{
		index = _FindEmptyIndexAfterIndex(table, index);
		if (index == -1)
			return;

		_SetCollisionKeyValuePairAtIndex(table, key, value, index);
	}

	tindex_t successfullyAddedIndex = _FindExistingIndexForKey(table, key);
	if (successfullyAddedIndex == -1)
		return;

	lst_SetValueForKey(table->iteratorKVList, index, key);
	if (lst_ValueForKey(table->iteratorKVList, key) != index)
	{
		_RemoveKeyValuePairAtIndex(table, index);
	}

}

#pragma mark Removing
void htbl_RemoveKey(struct HashTable *table, char *key)
{
	if (table == NULL)
		return;
	if (key == NULL)
		return;
	if (strlen(key) == 0)
		return;

	/*	Passing in a ptr to a key In a table will
		couse a loosing one after a removal of element, so we backup
		the key.
	 */
	size_t len = strlen(key);
	char backedUpKey[len + 1];
	memcpy(backedUpKey, key, len + 1);

	_RemoveKeyValuePair(table, backedUpKey);
}

static void _RemoveKeyValuePair(struct HashTable *table, char *key)
{
	tindex_t index = _FindExistingIndexForKey(table, key);
	if (index == -1)
		return;
	_RemoveKeyValuePairAtIndex(table, index);
}

static void _RemoveKeyValuePairAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	char *key = element->key;
	lst_RemoveElementWithKey(table->iteratorKVList, key);
	lst_RemoveElementWithKey(table->collisionsList, key);

	struct HashTableElement **array = table->array;
	_FreeElement(array[index]);
	array[index] = NULL;

	table->count--;
}

#pragma mark Setters
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

static void _SetKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	struct HashTableElement *element = _ElementAtIndex(table, index);
	if (element != NULL)
	{
		_SetValueInElement(element, value);
		return;
	}

	element = _MakeElement(key, value);
	if (element == NULL)
		return;

	struct HashTableElement **array = table->array;
	array[index] = element;
	table->count++;

}

static void _SetCollisionKeyValuePairAtIndex(struct HashTable *table, char *key, void *value, tindex_t index)
{
	_SetKeyValuePairAtIndex(table, key, value, index);

	struct HashTableElement *successfullyAddedElement = _ElementAtIndex(table, index);
	if (successfullyAddedElement == NULL)
		return;

	lst_SetValueForKey(table->collisionsList, index, key);
	if (lst_ValueForKey(table->collisionsList, key) == -1)
	{
		_RemoveKeyValuePairAtIndex(table, index);
	}
}

#pragma mark Getters
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

static inline struct HashTableElement *_ElementAtIndex(struct HashTable *table, tindex_t index)
{
	struct HashTableElement **array = table->array;
	return array[index];
}

#pragma mark Checkers
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

static bool _IsElementForKey(struct HashTableElement *element, char *key)
{
	char *elementKey = element->key;
	int compareResult = strncmp(elementKey, key, STRING_MAX_LEN);

	return compareResult ? 0 : 1;
}

#pragma mark Searching
static tindex_t _FindExistingIndexForKey(struct HashTable *table, char *key)
{
	// Search in hash index
	tindex_t desiredIndex = _HashFunction(key, table->hashLimit);
	bool containsDesiredElement = _IsElementAtIndexForKey(table, desiredIndex, key);
	if (containsDesiredElement != 0)
		return desiredIndex;

	// Search in collisions
	struct KeyValueList *collisionList = table->collisionsList;
	tindex_t collisionIndex = lst_ValueForKey(collisionList, key); // Will return -1 if not found
	if (collisionIndex != -1)
		return collisionIndex;

	return -1;
}

static tindex_t _FindEmptyIndexAfterIndex(struct HashTable *table, tindex_t startIndex)
{
	tindex_t currentIndex = startIndex;

	while (_IsElementAtIndexEmpty(table, currentIndex) == 0)
	{
		_loopIncrement(&currentIndex, 1, table->size);
		if (currentIndex == startIndex) /* If made a full circle */
			return -1;
	}

	return currentIndex;
}

#pragma mark Optimization
static void _OptimizeTable(struct HashTable *table)
{
	if ((float) table->count / table->size > 0.75)
		_ResizeTable(table, (size_t) table->size * 2);
}

static void _ResizeTable(struct HashTable *table, size_t newSize)
{
	struct HashTable *tmpTable = htbl_Create(newSize);
	if (tmpTable == NULL)
		return;

	struct HashTableIterator *iterator = htbl_IteratorForTable(table);
	if (iterator == NULL)
	{
		htbl_Free(tmpTable);
		return;
	}

	while (htbl_IsValidIterator(iterator))
	{
		htbl_SetValueForKey(tmpTable, iterator->value, iterator->key);
		iterator->next(iterator);
	}
	htbl_FreeIterator(iterator);

	_FreeTableContentsButLeaveStruct(table);
	memcpy(table, tmpTable, sizeof(struct HashTable));
	_FreeTableStructButLeakContents(tmpTable);
}

#pragma mark Stuff
size_t htbl_TableSize(struct HashTable *table)
{
	if (table == NULL)
		return 0;
	return (size_t) table->size;
}

size_t htbl_Count(struct HashTable *table)
{
	if (table == NULL)
		return 0;
	return (size_t) table->count;
}

tindex_t _HashFunction(char *key, tindex_t limit)
{
	size_t keyLength = strnlen(key, STRING_MAX_LEN);
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
static struct HashTableIterator *_InitIterator(struct HashTableIterator *iterator, struct HashTable *table);
static void _InvalidateIterator(struct HashTableIterator *iterator);
static void _IteratorNextFunction(struct HashTableIterator *iterator);

struct HashTableIterator *htbl_IteratorForTable(struct HashTable *table)
{
	if (table == NULL)
		return NULL;
	if (table->count == 0)
		return NULL;

	struct HashTableIterator *iterator = _AllocateIterator();
	if (iterator == NULL)
		return NULL;

	iterator = _InitIterator(iterator, table);

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

static struct HashTableIterator *_InitIterator(struct HashTableIterator *iterator, struct HashTable *table)
{
	struct KeyValueListIterator *listIterator = lst_IteratorForList(table->iteratorKVList);
	if (lst_IsIteratorValid(listIterator) == 0)
	{
		free(iterator->cheshire);
		free(iterator);
		return NULL;
	}

	iterator->table = table;
	iterator->cheshire->listIterator = listIterator;
	iterator->key = listIterator->key;
	iterator->value = table->array[listIterator->value]->value;
	iterator->next = _IteratorNextFunction;

	return iterator;
}

static void _IteratorNextFunction(struct HashTableIterator *iterator)
{
	if (htbl_IsValidIterator(iterator) == 0)
		return;

	struct KeyValueListIterator *listIterator = iterator->cheshire->listIterator;
	listIterator->next(listIterator);
	if (listIterator->key == NULL || strlen(listIterator->key) == 0)
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

	struct KeyValueList *listInsideIterator = iterator->cheshire->listIterator->list;
	struct KeyValueList *listInsideTable = iterator->table->iteratorKVList;
	if (listInsideIterator != listInsideTable)
		return 0;

	return 1;
}

void htbl_FreeIterator(struct HashTableIterator *iterator)
{
	if (iterator == NULL)
		return;

	lst_FreeIterator(iterator->cheshire->listIterator);
	free(iterator->cheshire);
	free(iterator);
}