#ifndef DirtyAllocation_H
#define DirtyAllocation_H

/* Less - more NULLs. 1 - always NULL */
#define AGGRESIVENESS 10

/* Prefixes, comma delimited.
	Example
	#define DIRTY_PREFIXES "my_class_","his_arm"
	#define DIRTY_PREFIXES_COUNT 2
	#define BLACKLISTED_PREFIXES "but_not_this_function_name"
	#define BLACKLISTED_PREFIXES_COUNT 1
 */
#define DIRTY_PREFIXES "htbl","lst"
#define DIRTY_PREFIXES_COUNT 2
#define BLACKLISTED_PREFIXES "_Resize"
#define BLACKLISTED_PREFIXES_COUNT 1

#endif