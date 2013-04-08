#include <stdlib.h>
#include <dlfcn.h>
#import <mm_malloc.h>

static void *__temporary_calloc(size_t x __attribute__((unused)), size_t y __attribute__((unused)))
{
	return NULL;
}

static void *(*r_calloc)(size_t, size_t) = NULL;
static void *(*r_malloc)(size_t) = NULL;

void initialize()
{
	r_malloc = dlsym(RTLD_NEXT, "malloc");
	r_calloc = __temporary_calloc;

	r_calloc = dlsym(RTLD_NEXT, "calloc");
}

void *malloc(size_t size)
{
	static int x = 0;
	++x;

	if (x % 10 == 0)
		return NULL;
	else
		return r_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
	/* Whatever */
	return r_calloc(nmemb, size);
}