#ifndef KeyValueList_h
#define KeyValueList_h
struct KeyValueList;
struct KeyValueListIteratorInternal;

struct KeyValueListIterator
{
	char *key;
	long value;

	void (*next)(struct KeyValueListIterator *);

	struct KeyValueList *list;
	struct KeyValueListIteratorInternal *cheshire;
};

struct KeyValueList *lst_CreateList();

void lst_SetValueForKey(struct KeyValueList *list, long value, char *key);
void lst_RemoveElementWithKey(struct KeyValueList *list, char *key);
long lst_ValueForKey(struct KeyValueList *list, char *key);
int8_t lst_IsIteratorValid(struct KeyValueListIterator *iterator);
void lst_Free(struct KeyValueList *list);

struct KeyValueListIterator *lst_IteratorForList(struct KeyValueList *list);
void lst_FreeIterator(struct KeyValueListIterator *iterator);

#endif