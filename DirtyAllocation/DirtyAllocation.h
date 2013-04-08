#ifndef DirtyAllocation_H
#define DirtyAllocation_H

/* Less - more NULLs. 1 - always NULL */
#define AGGRESSIVENESS 4

/* Prefixes, comma delimited.
	MonsterKill - return NULL always
	Dirty - return NULL if ( arc4random() % AGGRESSIVENESS )
	BlackListed - nomater what return memory. Beats MonsterKill.

	Example
	#define MONSTERKILL_PREFIXES "looser_function"
	#define MONSTERKILL_PREFIXES_COUNT 1

	#define DIRTY_PREFIXES "my_class_","his_arm"
	#define DIRTY_PREFIXES_COUNT 2

	#define BLACKLISTED_PREFIXES "but_not_this_holy_function_name"
	#define BLACKLISTED_PREFIXES_COUNT 1
 */

#define MONSTERKILL_PREFIXES "_Resize"
#define MONSTERKILL_PREFIXES_COUNT 0

#define DIRTY_PREFIXES "htbl","lst"
#define DIRTY_PREFIXES_COUNT 2

#define BLACKLISTED_PREFIXES
#define BLACKLISTED_PREFIXES_COUNT 0

#endif