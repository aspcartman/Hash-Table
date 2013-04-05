#import <string.h>
#import <stdlib.h>
#import "KeyValueList.h"

#pragma mark Private Header
struct KeyValueListElement
{
	char *key;
	long value;

	struct KeyValueListElement *next;
};

struct KeyValueListIteratorInternal
{
	struct KeyValueListElement *currentElement;
};

struct KeyValueList
{
	struct KeyValueListElement *firstElement;
};

struct KeyValueList *_allocateList();
struct KeyValueListElement *_CreateElement(char *key, long value);

#pragma mark Implementation

struct KeyValueList *lst_CreateList()
{
	struct KeyValueList *list = _allocateList();
	return list;
}

struct KeyValueList *_allocateList()
{
	size_t size = sizeof(struct KeyValueList);
	struct KeyValueList *list = calloc(1, size);
	return list;
}

void lst_SetValueForKey(struct KeyValueList *list, long value, char *key)
{
	if (list == NULL)
		return;
	if (key == NULL)
		return;

	struct KeyValueListElement *lastElement = list->firstElement;

	if (lastElement == NULL)
	{
		struct KeyValueListElement *newElement = _CreateElement(key, value);
		list->firstElement = newElement;
		return;
	}

	while (lastElement->next != NULL)
	{
		char *kKey = lastElement->key;
		int cmp = strcmp(kKey, key);
		if (cmp == 0)
		{
			lastElement->value = value;
			return;
		}
		lastElement = lastElement->next;
	}
	struct KeyValueListElement *newElement = _CreateElement(key, value);
	lastElement->next = newElement;
}

struct KeyValueListElement *_CreateElement(char *key, long value)
{
	size_t size = sizeof(struct KeyValueListElement);
	struct KeyValueListElement *element = malloc(size);
	if (element == NULL)
		return NULL;

	/* Memory is managed by the hashtable */
	element->key = key;
	element->value = value;
	element->next = NULL;

	return element;
}

void lst_RemoveElementWithKey(struct KeyValueList *list, char *key)
{
	if (list == NULL)
		return;
	if (key == NULL)
		return;

	struct KeyValueListElement *element = list->firstElement;

	if (element != NULL)
	{
		char *kKey = element->key;
		int cmp = strcmp(key, kKey);
		if (cmp == 0)
		{
			list->firstElement = element->next;
			return;
		}
	}

	while (element->next != NULL)
	{
		char *kKey = element->next->key;
		int cmp = strcmp(key, kKey);
		if (cmp == 0)
		{
			struct KeyValueListElement *replacement = element->next;
			element->next = replacement->next;
			free(replacement);
			return;
		}
		element = element->next;
	}
}

long lst_ValueForKey(struct KeyValueList *list, char *key)
{
	if (list == NULL)
		return -1;
	if (key == NULL)
		return -1;
	struct KeyValueListElement *element = list->firstElement;
	while (element != NULL)
	{
		char *kKey = element->key;
		int cmp = strcmp(kKey, key);
		if (cmp == 0)
		{
			return element->value;
		}
		element = element->next;
	}
	return -1;
}

void lst_Free(struct KeyValueList *list)
{
	if (list == NULL)
		return;

	struct KeyValueListElement *element = list->firstElement;

	while (element)
	{
		struct KeyValueListElement *nElem = element->next;
		free(element);
		element = nElem;
	}

	free(list);
}

#pragma mark Iterator
struct KeyValueListIterator *_AllocateIterator();
void _InitIterator(struct KeyValueListIterator *iterator, struct KeyValueList *list);
void _IteratorNextFunction(struct KeyValueListIterator *iterator);

struct KeyValueListIterator *lst_IteratorForList(struct KeyValueList *list)
{
	if (list == NULL)
		return NULL;
	if (list->firstElement == NULL)
		return NULL;

	struct KeyValueListIterator *iterator = _AllocateIterator();
	if (iterator == NULL)
		return NULL;

	_InitIterator(iterator, list);

	return iterator;
}

struct KeyValueListIterator *_AllocateIterator()
{
	struct KeyValueListIterator *iterator = malloc(sizeof(struct KeyValueListIterator));
	if (iterator == NULL)
		return NULL;

	struct KeyValueListIteratorInternal *cheshire = malloc(sizeof(struct KeyValueListIteratorInternal));
	if (cheshire == NULL)
	{
		free(iterator);
		return NULL;
	}
	iterator->cheshire = cheshire;

	return iterator;
}

void _InitIterator(struct KeyValueListIterator *iterator, struct KeyValueList *list)
{
	iterator->key = list->firstElement->key;
	iterator->value = list->firstElement->value;
	iterator->next = _IteratorNextFunction;
	iterator->list = list;
	iterator->cheshire->currentElement = list->firstElement;
}

void _IteratorNextFunction(struct KeyValueListIterator *iterator)
{
	if (iterator->key == NULL)
		return;

	struct KeyValueListElement *nextElement = iterator->cheshire->currentElement->next;
	if (nextElement == NULL)
	{
		iterator->key = NULL;
		iterator->value = 0;
		iterator->cheshire->currentElement = NULL;
		return;
	}

	iterator->cheshire->currentElement = nextElement;
	iterator->key = nextElement->key;
	iterator->value = nextElement->value;
}

void lst_FreeIterator(struct KeyValueListIterator *iterator)
{
	if (iterator == NULL)
		return;

	free(iterator->cheshire);
	free(iterator);
}