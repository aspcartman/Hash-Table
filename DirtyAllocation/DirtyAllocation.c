#include <stdlib.h>
#include <dlfcn.h>
#import <mm_malloc.h>
#import <stdio.h>
#include <execinfo.h>
#import <string.h>
#include "DirtyAllocation.h"


struct DA_Symbols
{
	char *dirtyPrefixes[DIRTY_PREFIXES_COUNT];
	char *blackList[BLACKLISTED_PREFIXES_COUNT];
};
static struct DA_Symbols smbls = {{DIRTY_PREFIXES}, {BLACKLISTED_PREFIXES}};

static void *(*r_calloc)(size_t, size_t) = NULL;
static void *(*r_malloc)(size_t) = NULL;

static bool _shouldReturnNull();
static int getBacktrace(Dl_info *output, size_t len);
static bool backtraceContainsBlackListedSymbol(Dl_info *backtrace, int backtraceSize);
static bool backtraceContainsDirtyPrefixes(Dl_info *backtrace, int backtraceSize);
static bool backtraceContainsSymbolStartingWith(Dl_info *backtrace, int backtraceSize, char *string);
static void print_trace(void);

void *malloc(size_t size)
{
	if (r_malloc == NULL)
		r_malloc = dlsym(RTLD_NEXT, "malloc");

	static int x = 0;
	++x;

	if (x % AGGRESIVENESS == 0 && _shouldReturnNull())
		return NULL;
	else
		return r_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
	if (r_calloc == NULL)
		r_calloc = dlsym(RTLD_NEXT, "calloc");

	static int x = 0;
	++x;

	if (x % AGGRESIVENESS == 0 && _shouldReturnNull())
		return NULL;
	else
		return r_calloc(nmemb, size);
}

#pragma mark Rutines

static bool _shouldReturnNull()
{
	Dl_info backtrace[10] = {0};
	int backtraceLen = getBacktrace(backtrace, 10);

	bool containsBlackListed = backtraceContainsBlackListedSymbol(backtrace, backtraceLen);
	if (containsBlackListed == true)
		return false;

	bool containsDirty = backtraceContainsDirtyPrefixes(backtrace, backtraceLen);
	if (containsDirty == true)
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

static bool backtraceContainsBlackListedSymbol(Dl_info *backtrace, int backtraceSize)
{

	for (int i = 0; i < BLACKLISTED_PREFIXES_COUNT; ++i)
	{
		char *symbolPrefix = smbls.blackList[i];
		bool contains = backtraceContainsSymbolStartingWith(backtrace, backtraceSize, symbolPrefix);
		if (contains == true)
			return 1;
	}
	return 0;
}

static bool backtraceContainsDirtyPrefixes(Dl_info *backtrace, int backtraceSize)
{
	for (int i = 0; i < DIRTY_PREFIXES_COUNT; ++i)
	{
		char *symbolPrefix = smbls.dirtyPrefixes[i];
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
