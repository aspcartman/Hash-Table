#include "DirtyAllocation.h"
#include <stdlib.h>
#include <dlfcn.h>
#import <mm_malloc.h>
#import <stdio.h>
#include <execinfo.h>
#import <string.h>
#import <assert.h>


struct DA_Symbols
{
	char *monsterKill[MONSTERKILL_PREFIXES_COUNT];
	int monsterKillCount;

	char *dirty[DIRTY_PREFIXES_COUNT];
	int dirtyCount;

	char *blackList[BLACKLISTED_PREFIXES_COUNT];
	int blackListCount;

	char *traceList[TRACELIST_PREFIXES_COUNT];
	int traceListCount;
};

static struct DA_Symbols smbls = {
		{MONSTERKILL_PREFIXES}, MONSTERKILL_PREFIXES_COUNT,
		{DIRTY_PREFIXES}, DIRTY_PREFIXES_COUNT,
		{BLACKLISTED_PREFIXES}, BLACKLISTED_PREFIXES_COUNT,
		{TRACELIST_PREFIXES}, TRACELIST_PREFIXES_COUNT};

void **traceListArray = NULL;
size_t traceListArrayLen = 0;

static void *(*r_calloc)(size_t, size_t) = NULL;
static void *(*r_malloc)(size_t) = NULL;
static void *(*r_free)(void *) = NULL;

static bool _shouldReturnNull();
static bool _shouldTraceAllocation();
static int getBacktrace(Dl_info *output, size_t len);
static bool backtraceContainsMonsterKillSymbol(Dl_info *backtrace, int backtraceSize);
static bool backtraceContainsBlackListedSymbol(Dl_info *backtrace, int backtraceSize);
static bool backtraceContainsDirtyPrefixes(Dl_info *backtrace, int backtraceSize);
static bool backtraceContainsPrefixesFromArray(Dl_info *backtrace, int backtraceSize, char **prefixArray, int count);
static bool backtraceContainsSymbolStartingWith(Dl_info *backtrace, int backtraceSize, char *string);
static bool backtraceContainsTracePrefixes(Dl_info *backtrace, int backtraceSize);

static void print_trace(void);

void addAllocationToTrace(void *ptr);
void removeAllocationFromTrace(void *ptr);

void *malloc(size_t size)
{
	if (r_malloc == NULL)
		r_malloc = dlsym(RTLD_NEXT, "malloc");

	if (_shouldReturnNull())
		return NULL;

	void *memporyPtr = r_malloc(size);

	if (_shouldTraceAllocation() && memporyPtr != NULL)
		addAllocationToTrace(memporyPtr);

	return memporyPtr;
}

void *calloc(size_t nmemb, size_t size)
{
	if (r_calloc == NULL)
		r_calloc = dlsym(RTLD_NEXT, "calloc");

	if (_shouldReturnNull())
		return NULL;

	void *memporyPtr = r_calloc(nmemb, size);

	if (_shouldTraceAllocation() && memporyPtr != NULL)
		addAllocationToTrace(memporyPtr);

	return memporyPtr;
}

void free(void *ptr)
{
	if (r_free == NULL)
		r_free = dlsym(RTLD_NEXT, "free");

	if (_shouldTraceAllocation())
		removeAllocationFromTrace(ptr);
	r_free(ptr);
}
#pragma mark Rutines

static bool _shouldReturnNull()
{
	Dl_info backtrace[10] = {0};
	int backtraceLen = getBacktrace(backtrace, 10);

	bool containsMonsterKill = backtraceContainsMonsterKillSymbol(backtrace, backtraceLen);
	bool containsBlackListed = backtraceContainsBlackListedSymbol(backtrace, backtraceLen);
	bool containsDirty = backtraceContainsDirtyPrefixes(backtrace, backtraceLen);

	if (containsBlackListed == true)
		return false;
	if (containsMonsterKill == true)
		return true;
	if (containsDirty == true)
		return (arc4random() % AGGRESSIVENESS) ? false : true;

	return false;
}

static bool _shouldTraceAllocation()
{
	if (traceListArray == NULL)
		return false;

	Dl_info backtrace[10] = {0};
	int backtraceLen = getBacktrace(backtrace, 10);

	bool containsTraceSymbol = backtraceContainsTracePrefixes(backtrace, backtraceLen);

	if (containsTraceSymbol)
		return true;
	return false;
}

static int getBacktrace(Dl_info *output, size_t len)
{
	void *returnAddresses[len];
	int size = backtrace(returnAddresses, len);

	int finalSize = 0;
	for (int i = 0; i < size; ++i, ++finalSize)
	{
		void *addr = returnAddresses[i];

		Dl_info info;
		int foundInfo = dladdr(addr, &info);
		if (foundInfo == 0)
		{
			--finalSize;
			continue;
		}

		output[finalSize] = info;
	}

	return finalSize;
}

static bool backtraceContainsMonsterKillSymbol(Dl_info *backtrace, int backtraceSize)
{
	return backtraceContainsPrefixesFromArray(backtrace, backtraceSize, smbls.monsterKill, smbls.monsterKillCount);
}

static bool backtraceContainsBlackListedSymbol(Dl_info *backtrace, int backtraceSize)
{
	return backtraceContainsPrefixesFromArray(backtrace, backtraceSize, smbls.blackList, smbls.blackListCount);
}

static bool backtraceContainsDirtyPrefixes(Dl_info *backtrace, int backtraceSize)
{
	return backtraceContainsPrefixesFromArray(backtrace, backtraceSize, smbls.dirty, smbls.dirtyCount);
}

static bool backtraceContainsTracePrefixes(Dl_info *backtrace, int backtraceSize)
{
	return backtraceContainsPrefixesFromArray(backtrace, backtraceSize, smbls.traceList, smbls.traceListCount);
}

static bool backtraceContainsPrefixesFromArray(Dl_info *backtrace, int backtraceSize, char **prefixArray, int count)
{
	for (int i = 0; i < count; ++i)
	{
		char *symbolPrefix = prefixArray[i];
		bool contains = backtraceContainsSymbolStartingWith(backtrace, backtraceSize, symbolPrefix);
		if (contains == true)
			return 1;
	}
	return 0;
}

static bool backtraceContainsSymbolStartingWith(Dl_info *backtrace, int backtraceSize, char *string)
{
	for (int i = 0; i < backtraceSize; ++i)
	{
		Dl_info info = backtrace[i];
		if (info.dli_sname == NULL)
			continue;

		size_t len = strlen(string);
		int cmp = strncmp(string, info.dli_sname, len);
		if (cmp == 0)
			return 1;
	}
	return 0;
}

void print_trace(void)
{
	void *array[10];
	char **strings;
	size_t i;

	int size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	printf("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf("%s\n", strings[i]);
	fpurge(stdout);
	free(strings);
}

#pragma mark TraceListArray
void StartTracing()
{
	traceListArray = r_calloc(0, sizeof(void *));
}

void addAllocationToTrace(void *ptr)
{
	traceListArray = realloc(traceListArray, traceListArrayLen + 1);
	traceListArray[traceListArrayLen] = ptr;
	traceListArrayLen += 1;
}

void removeAllocationFromTrace(void *ptr)
{
	for (size_t i = 0; i < traceListArrayLen; ++i)
	{
		if (traceListArray[i] == ptr)
		{
			traceListArray[i] = NULL;
			return;
		}
	}
}

void StopTracing()
{
	for (int i = 0; i < traceListArrayLen; ++i)
	{
		if (traceListArray[i] != 0)
		assert(0);
	}
	free(traceListArray);
	traceListArray = NULL;
	traceListArrayLen = 0;
}