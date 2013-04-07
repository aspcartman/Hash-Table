//
// Created by aspcartman on 04.04.13.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "NSString+RandomString.h"


@implementation NSString (RandomString)
+ (NSString *) randomStringWithLength:(NSUInteger)length;
{
	static NSString *letters = @"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	NSMutableString *randomString = [NSMutableString stringWithCapacity:length];

	for (NSUInteger i = 0; i < length; i++)
	{
		[randomString appendFormat:@"%C", [letters characterAtIndex:arc4random() % [letters length]]];
	}

	return [NSString stringWithString:randomString];
}

@end