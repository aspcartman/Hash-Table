//
//  HashTableTests.m
//  HashTableTests
//
//  Created by ASPCartman on 04.04.13.
//
//

#include <dlfcn.h>
#import "HashTableTests.h"
#import "HashTable.h"
#import "NSString+RandomString.h"

#define KEY_LEN 4
#define TABLE_LEN 10

@interface HashTableTests ()
@property(assign) struct HashTable *table;
@end

@implementation HashTableTests

- (void) setUp
{
//	[self raiseAfterFailure];
	self.table = htbl_Create(TABLE_LEN);
	void (*startTracingLeaks)() = dlsym(RTLD_DEFAULT, "StartTracing");
	if (startTracingLeaks != NULL)
		startTracingLeaks();
}

- (void) tearDown
{
	htbl_Free(self.table);
	void (*stopTracingLeaks)() = dlsym(RTLD_DEFAULT, "StopTracing");
	if (stopTracingLeaks != NULL)
		stopTracingLeaks();
}

#pragma mark Adding
- (void) testAdd1Object
{
	[self addObjectsToTable:1];
}

- (void) testAddTableSizeObjects
{
	size_t tableSize = htbl_TableSize(self.table);
	[self addObjectsToTable:(NSUInteger) tableSize];
}

- (void) testAdd1e2Objects // Needs resizing to work
{
	[self addObjectsToTable:100];
}

- (void) testAdd1e3Objects // Needs resizing to work
{
	[self addObjectsToTable:1000];
}

- (NSMutableDictionary *) addObjectsToTable:(NSUInteger)count
{
	NSMutableDictionary *idealDictionary = [NSMutableDictionary dictionaryWithCapacity:count];
	for (NSUInteger i = 0; i < count; ++i)
	{
		NSString *keyString = [NSString randomStringWithLength:KEY_LEN];
		if ([idealDictionary objectForKey:keyString])
		{
			--i;
			continue;
		}

		char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];
		NSDate *value = [NSDate date];

		htbl_SetValueForKey(self.table, (__bridge void *) value, key);
		idealDictionary[keyString] = value;
	}

	[self compareToDict:idealDictionary];

	return idealDictionary;
}

#pragma mark Removing
- (void) testAddAndRemove1Object
{
	[self addAndRemoveObjectsToTable:1];
}

- (void) testAddAndRemove1e2Objects
{
	[self addAndRemoveObjectsToTable:100];
}

- (void) testAddAndRemove1e3Objects
{
	[self addAndRemoveObjectsToTable:1000];
}

- (void) addAndRemoveObjectsToTable:(NSUInteger)count
{
	NSMutableDictionary *idealDictionary = [self addObjectsToTable:count];

	for (NSUInteger i = 0; i < count; ++i)
	{
		NSString *keyString = idealDictionary.allKeys[i];
		char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

		htbl_RemoveKey(self.table, key);
	}

	// HandCheck
	for (NSUInteger i = 0; i < count; ++i)
	{
		NSString *keyString = idealDictionary.allKeys[i];
		char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

		void *rValue = htbl_ValueForKey(self.table, (char *) &key);
		STAssertEquals(rValue, NULL, @"Returned object must be NULL");
	}
}

- (void) testRandomAddAndRemove1e2times
{
	[self randomAddAndRemove:100];
}

- (void) testRandomAddAndRemove1e3times
{
	[self randomAddAndRemove:1000];
}

#pragma mark Change
- (void) testReplaceValueForKey
{
	char key[] = "Any Key";
	htbl_SetValueForKey(self.table, (void *) 1, key);
	htbl_SetValueForKey(self.table, (void *) 2, key);

	void *value = htbl_ValueForKey(self.table, key);
	STAssertEquals(value, (void *) 2, @"Value should've been changed");
}

#pragma mark Random Adding and Removing
- (void) randomAddAndRemove:(NSUInteger)iterations
{
	enum
	{
		kAdd = 0, kRemove = 1
	} action = kAdd;
	NSMutableDictionary *idealDictionary = [NSMutableDictionary dictionaryWithCapacity:iterations];
	NSUInteger i = 0;
	for (i = 0; i < iterations; ++i)
	{
		if (action == kAdd)
		{
			NSString *keyString = [NSString randomStringWithLength:KEY_LEN];
			char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];
			NSDate *value = [NSDate date];

			htbl_SetValueForKey(self.table, (__bridge void *) value, key);
			idealDictionary[keyString] = value;
		}
		else if (action == kRemove)
		{
			if (idealDictionary.count == 0)
			{
				action = kAdd;
				continue;
			}
			NSString *keyString = idealDictionary.allKeys[arc4random() % idealDictionary.count];
			char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

			htbl_RemoveKey(self.table, key);
			[idealDictionary removeObjectForKey:keyString];
		}

		[self compareToDict:idealDictionary];

		action = arc4random() % 2;
	}
}

#pragma mark Wrong Arguments
- (void) testWrongArguments
{
	static char *const whatever = "Whatever";
	static char *const emptyString = "";

	htbl_SetValueForKey(NULL, whatever, whatever);
	htbl_SetValueForKey(self.table, NULL, whatever);
	htbl_SetValueForKey(self.table, whatever, emptyString);
	htbl_SetValueForKey(self.table, whatever, NULL);

	htbl_RemoveKey(NULL, whatever);
	htbl_RemoveKey(self.table, emptyString);
	htbl_RemoveKey(self.table, NULL);

	htbl_ValueForKey(NULL, whatever);
	htbl_ValueForKey(self.table, emptyString);
	htbl_ValueForKey(self.table, NULL);

	htbl_TableSize(NULL);
	htbl_Count(NULL);
	htbl_Count(self.table);
	htbl_Free(NULL);

	htbl_IteratorForTable(NULL);
	htbl_IsValidIterator(NULL);
	htbl_FreeIterator(NULL);
}

#pragma mark Collisions
extern long _HashFunction(char *key, long limit);
#define COLISIONS 30

// TODO: Find a way to investigate rare failures. Is it a test or hashtable?
- (void) testCollisions
{
	NSMutableDictionary *idealDictionary = [NSMutableDictionary dictionaryWithCapacity:11];
	NSString *keyString = @"hh";
	char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];
	size_t sizeOfTable = htbl_TableSize(self.table);
	if (sizeOfTable == 0)
		return;

	long index = _HashFunction(key, htbl_TableSize(self.table));

	htbl_SetValueForKey(self.table, (__bridge void *) keyString, key);
	[idealDictionary setObject:keyString forKey:keyString];

	for (int i = 0; i < COLISIONS; ++i)
	{
		NSString *newKeyString;
		char *newKey;
		long newIndex;
		do
		{
			newKeyString = [NSString randomStringWithLength:2];
			if ([idealDictionary objectForKey:newKeyString])
				continue;

			newKey = (char *) [newKeyString cStringUsingEncoding:NSASCIIStringEncoding];
			newIndex = _HashFunction(key, htbl_TableSize(self.table));
		} while (newIndex != index);

		htbl_SetValueForKey(self.table, (__bridge void *) newKeyString, newKey);
		[idealDictionary setObject:newKeyString forKey:newKeyString];

		[self compareToDict:idealDictionary];
	}
}

#pragma mark Iterator
- (void) testIterator
{
	NSMutableDictionary *idealDictionary = [self addObjectsToTable:1000];

	// Test if keys match their values
	struct HashTableIterator *iterator = htbl_IteratorForTable(self.table);
	while (htbl_IsValidIterator(iterator))
	{
		void *iteratorValue = iterator->value;
		void *tableValue = htbl_ValueForKey(self.table, iterator->key);

		STAssertEquals(iteratorValue, tableValue, @"Key %s", iterator->key);
		NSString *keyString = [NSString stringWithCString:iterator->key encoding:NSASCIIStringEncoding];
		[idealDictionary removeObjectForKey:keyString];

		iterator->next(iterator);
	}

	STAssertEquals(idealDictionary.count, (NSUInteger) 0, @"Iterator didn't iterate to the end");

	htbl_FreeIterator(iterator);
}

- (void) testIteratorOnRemoving
{
	NSMutableDictionary *idealDictionary = [self addObjectsToTable:100];

	// Test if keys match their values
	struct HashTableIterator *iterator = htbl_IteratorForTable(self.table);
	while (htbl_IsValidIterator(iterator))
	{
		NSString *keyString = [NSString stringWithCString:iterator->key encoding:NSASCIIStringEncoding];
		[idealDictionary removeObjectForKey:keyString];

		htbl_RemoveKey(self.table, iterator->key);

		[self compareToDict:idealDictionary];

		iterator->next(iterator);
	}

	STAssertEquals(idealDictionary.count, (NSUInteger) 0, @"Iterator didn't iterate to the end");

	htbl_FreeIterator(iterator);
}

- (void) testIteratorOnTableGrowth
{
	[self addObjectsToTable:20];
	struct HashTableIterator *iterator = htbl_IteratorForTable(self.table);
	if (iterator == NULL)
		return;

	while (htbl_IsValidIterator(iterator))
	{
		[self addObjectsToTable:1];
		iterator->next(iterator);
	}
	htbl_FreeIterator(iterator);
}

#pragma mark Helper Functions
- (void) compareToDict:(NSDictionary *)dict
{
	for (NSString *keyString in dict)
	{
		char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

		id myValue = (__bridge id) htbl_ValueForKey(self.table, key);
		id idealValue = dict[keyString];

		STAssertEquals(myValue, idealValue, @"Value in ideal and mine dictionaries must be equal");
	}
}

@end
