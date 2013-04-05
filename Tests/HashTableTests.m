//
//  HashTableTests.m
//  HashTableTests
//
//  Created by ASPCartman on 04.04.13.
//
//

#import "HashTableTests.h"
#import "HashTable.h"
#import "NSString+RandomString.h"

#define KEY_LEN 10

@interface HashTableTests ()
@property(assign) struct HashTable *table;
@end

@implementation HashTableTests
- (void) setUp
{
	self.table = htbl_Create();
}

- (void) tearDown
{
	htbl_Free(self.table);
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

#pragma mark Random Adding and Removing
- (void) testRandomAddingAndRemoving
{
#define ITERATIONS 10000
	enum
	{
		kAdd = 0, kRemove = 1
	} action = kAdd;
	NSMutableDictionary *idealDictionary = [NSMutableDictionary dictionaryWithCapacity:ITERATIONS];
	NSUInteger i = 0;
	for (i = 0; i < ITERATIONS; ++i)
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
				continue;

			NSString *keyString = idealDictionary.allKeys[arc4random() % idealDictionary.count];
			char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

			htbl_RemoveKey(self.table, key);
			[idealDictionary removeObjectForKey:keyString];
		}

		[self compareToDict:idealDictionary];

		action = arc4random() % 2;
	}
	NSLog(@"Made %u iterations", (unsigned int) i);
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

	htbl_Free(NULL);
}

#pragma mark Memory Allocations Fails

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
