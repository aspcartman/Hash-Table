#import <string.h>
#import <stdlib.h>
#import "KeyValueList.h"

#pragma mark Private Header
struct ListElement{
	char *key;
	long value;

	struct ListElement *next;
};

struct KeyValueList
{
	struct ListElement *firstElement;
	size_t count;
};

struct KeyValueList *_allocateList();
struct ListElement *_CreateElement(char *key, long value);

#pragma mark Implementation

struct KeyValueList *lst_CreateList()
{
	struct KeyValueList *list = _allocateList();
	return list;
}

struct KeyValueList *_allocateList()
{
	size_t size = sizeof(struct KeyValueList);
	struct KeyValueList *list = calloc(1,size);
	return list;
}

void lst_SetValueForKey(struct KeyValueList *list, long value, char *key)
{
	if (list == NULL)
		return;
	if (key == NULL)
		return;

	struct ListElement *lastElement = list->firstElement;

	if (lastElement == NULL)
	{
		struct ListElement *newElement = _CreateElement(key, value);
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
	struct ListElement *newElement = _CreateElement(key, value);
	lastElement->next = newElement;
}

struct ListElement *_CreateElement(char *key, long value)
{
	size_t size = sizeof(struct ListElement);
	struct ListElement *element = malloc(size);
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
	if (key	 == NULL)
		return;

	struct ListElement *element = list->firstElement;

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
			struct ListElement *relement = element->next;
			element->next = relement->next;
			free(relement);
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
	struct ListElement *element = list->firstElement;
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

	struct ListElement *element = list->firstElement;

	while (element)
	{
		struct ListElement *nElem = element->next;
		free(element);
		element = nElem;
	}

	free(list);
}