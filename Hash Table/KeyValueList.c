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
	struct KeyValueListElement *nextElement;
};

struct KeyValueList
{
	struct KeyValueListElement *firstElement;
};

static struct KeyValueList *_allocateList();
static struct KeyValueListElement *_CreateElement(char *key, long value);

#pragma mark Implementation

struct KeyValueList *lst_CreateList()
{
	struct KeyValueList *list = _allocateList();
	return list;
}

static struct KeyValueList *_allocateList()
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

	if (list->firstElement == NULL)
	{
		struct KeyValueListElement *newElement = _CreateElement(key, value);
		list->firstElement = newElement;
		return;
	}

	struct KeyValueListElement *element = list->firstElement;
	for (; ;)
	{
		int cmp = strcmp(element->key, key);
		if (cmp == 0)
		{
			element->value = value;
			return;
		}

		if (element->next == NULL)
			break;

		element = element->next;
	}

	struct KeyValueListElement *newElement = _CreateElement(key, value);
	if (newElement == NULL)
		return;
	element->next = newElement;
}

struct KeyValueListElement *_CreateElement(char *key, long value)
{
	size_t size = sizeof(struct KeyValueListElement);
	struct KeyValueListElement *element = malloc(size);
	if (element == NULL)
		return NULL;

	/* We don't copy key string */
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
	if (list->firstElement == NULL)
		return;

	struct KeyValueListElement *element = list->firstElement;

	int cmp = strcmp(key, element->key);
	if (cmp == 0)
	{
		struct KeyValueListElement *elementToDelete = list->firstElement;
		list->firstElement = list->firstElement->next;
		free(elementToDelete);
		return;
	}

	while (element->next != NULL)
	{
		char *kKey = element->next->key;
		cmp = strcmp(key, kKey);
		if (cmp == 0)
		{
			struct KeyValueListElement *elementToDelete = element->next;
			element->next = element->next->next;
			free(elementToDelete);
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
void _InvalidateIterator(struct KeyValueListIterator *iterator);
void _SetIteratorToElement(struct KeyValueListIterator *iterator, struct KeyValueListElement *element);

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
	_SetIteratorToElement(iterator, list->firstElement);
	iterator->next = _IteratorNextFunction;
	iterator->list = list;
}

void _SetIteratorToElement(struct KeyValueListIterator *iterator, struct KeyValueListElement *element)
{
	iterator->key = element->key;
	iterator->value = element->value;
	iterator->cheshire->currentElement = element;
	iterator->cheshire->nextElement = element->next;
}

void _IteratorNextFunction(struct KeyValueListIterator *iterator)
{
	if (lst_IsIteratorValid(iterator) == 0)
		return;

	struct KeyValueListElement *nextElement = iterator->cheshire->nextElement;
	if (nextElement == NULL)
	{
		_InvalidateIterator(iterator);
		return;
	}

	_SetIteratorToElement(iterator, nextElement);
}

void _InvalidateIterator(struct KeyValueListIterator *iterator)
{
	iterator->key = NULL;
	iterator->value = 0;
	iterator->cheshire->currentElement = NULL;
}

int8_t lst_IsIteratorValid(struct KeyValueListIterator *iterator)
{
	if (iterator == NULL)
		return 0;

	if (iterator->cheshire->currentElement == NULL)
		return 0;
	else
		return 1;
}

void lst_FreeIterator(struct KeyValueListIterator *iterator)
{
	if (iterator == NULL)
		return;

	free(iterator->cheshire);
	free(iterator);
}