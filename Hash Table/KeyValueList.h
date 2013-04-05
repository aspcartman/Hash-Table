#ifndef KeyValueList_h
#define KeyValueList_h

struct KeyValueList *lst_CreateList();
void lst_SetValueForKey(struct KeyValueList *list, long value, char *key);
void lst_RemoveElementWithKey(struct KeyValueList *list, char *key);
long lst_ValueForKey(struct KeyValueList *list, char *key);
void lst_Free(struct KeyValueList *list);


#endif