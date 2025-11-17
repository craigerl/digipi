/*
 *	hmalloc.c
 *
 *	Replacements for malloc, realloc and free, which never fail,
 *	and might keep statistics on memory allocation...
 */

#include <unistd.h>
#include <string.h>

#include "hmalloc.h"
#include "log.h"

int mem_panic = 0;

long int hmallocs = 0;
long int hmallocs_b = 0;
long int hcallocs = 0;
long int hcallocs_b = 0;
long int hfrees = 0;
long int hreallocs = 0;
long int hstrdups = 0;

void *hmalloc(size_t size)
{
	void *p;
	
	if (!(p = malloc(size))) {
		if (mem_panic)
			exit(1);	/* To prevent a deadlock */
		mem_panic = 1;
		do_log(L_CRIT, "hmalloc: Out of memory! Could not allocate %d bytes.", size);
		exit(1);
	}
	
	hmallocs++;
	hmallocs_b += size;
	return p;
}

void *hcalloc(size_t nmemb, size_t size)
{
	void *p;
	
	if (!(p = calloc(nmemb, size))) {
		if (mem_panic)
			exit(1);	/* To prevent a deadlock */
		mem_panic = 1;
		do_log(L_CRIT, "hcalloc: Out of memory! Could not allocate %d*%d bytes.", nmemb, size);
		exit(1);
	}
	
	hcallocs++;
	hcallocs_b += size;
	return p;
}

void *hrealloc(void *ptr, size_t size)
{
	void *p;
	
	if (!(p = realloc(ptr, size))) {
		if (mem_panic)
			exit(1);
		mem_panic = 1;
		do_log(L_CRIT, "hrealloc: Out of memory! Could not reallocate %d bytes.", size);
		exit(1);
	}
	
	hreallocs++;
	return p;
}

void hfree(void *ptr)
{
	free(ptr);
	hfrees++;
}

char *hstrdup(const char *s)
{
	char *p;
	
	p = hmalloc(strlen(s)+1);
	strcpy(p, s);
	
	hstrdups++;
	return p;
}

