//
// Created by aspcartman on 05.04.13.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "KeyValueListTests.h"
#import "NSString+RandomString.h"
#import "KeyValueList.h"
@interface  KeyValueListTests ()
@property (assign) struct KeyValueList *list;
@end
@implementation KeyValueListTests

- (void) setUp
{
	self.list = lst_CreateList();
}

- (void) tearDown
{
	lst_Free(self.list);
}

#pragma mark Adding
- (void) testAdd1Pair
{
	[self addObjectsToList:1];
}

- (void) testAdd1e2Pairs
{
	[self addObjectsToList:100];
}

#pragma mark Add And Remove
- (void) testAddAndRemove1Pairs
{
	[self addAndRemoveObjects:1];
}

- (void) testAddAndRemove1e2Pairs
{
	[self addAndRemoveObjects:100];
}

- (void) testRandomAddingAndRemoving
{
#define ITERATIONS 10000
	enum
	{
		kAdd = 0, kRemove = 1
	} action = kAdd;
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:ITERATIONS];

	for (NSUInteger i = 0; i < ITERATIONS; ++i)
	{
		if (action == kAdd)
		{
			NSString *keyString = [NSString randomStringWithLength:10];
			char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];
			long value = i;

			lst_SetValueForKey(self.list, value, key);
			array[i] = keyString;
		}
		else if (action == kRemove)
		{
			bool hasKeys = NO;
			for (NSString *key in array)
			{
				if(![key isEqualToString:@""])
					hasKeys = YES;
			}
			if (hasKeys == NO)
			{
				action = kAdd;
				i--;
				continue;
			}

			NSString *keyString = @"";
			NSUInteger index;
			while([keyString isEqualToString:@""])
			{
				index = arc4random()%array.count;
				keyString = array[index];
			}
			char *key = (char *) [keyString cStringUsingEncoding:NSASCIIStringEncoding];

			lst_RemoveElementWithKey(self.list, key);
			array[index] = @"";
			array[i] = @"";
		}else{
			STFail(@"WTF?");
		}

		[self compareToArray:array];

		action = arc4random() % 2;
	}
}

#pragma mark Wrong Arguments
- (void) testWrongArguments
{
	lst_SetValueForKey(NULL, 1, "");
	lst_SetValueForKey(self.list, 1, NULL);

	lst_RemoveElementWithKey(NULL, "");
	lst_RemoveElementWithKey(self.list, NULL);

	lst_ValueForKey(NULL, "");
	lst_ValueForKey(self.list, NULL);

	lst_Free(NULL);
}
#pragma mark Methods
- (void) addAndRemoveObjects:(NSUInteger)count
{
	NSMutableArray *array = [self addObjectsToList:count];

	for (NSUInteger i = 0; i < count; ++i)
	{
		NSString *keyString = array[i];
		char *key = (char*)[keyString cStringUsingEncoding:NSASCIIStringEncoding];
		long value = (long)i;

		lst_RemoveElementWithKey(self.list, key);
		long rValue = lst_ValueForKey(self.list, key);
		STAssertEquals(rValue,(long)-1, @"After a removal value for this key should be -1");

		array[i] = @"";
		[self compareToArray:array];
	}
}

- (NSMutableArray *) addObjectsToList:(NSUInteger)count
{
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:count];

	for (NSUInteger i = 0; i < count; ++i )
	{
		NSString *keyString = [NSString randomStringWithLength:10];
		char *key = (char*)[keyString cStringUsingEncoding:NSASCIIStringEncoding];
		long value = (long)i;

		array[i] = keyString;
		lst_SetValueForKey(self.list, value, key);
		[self compareToArray:array];
	}

	return array;
}

- (void) compareToArray:(NSArray*)array
{
	for (NSUInteger i = 0; i < array.count; ++i )
	{
		NSString *keyString = array[i];
		if (keyString == @"")
			continue;
		char *key = (char*)[keyString cStringUsingEncoding:NSASCIIStringEncoding];
		long value = (long)i;

		long rValue = lst_ValueForKey(self.list, key);
		STAssertEquals(rValue, value, @"List returned wrong value for key");
	}
}
@end