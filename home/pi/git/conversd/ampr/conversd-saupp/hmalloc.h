
#ifndef HMALLOC_H
#define HMALLOC_H

#include <stdlib.h>

/*
 *	Replacements for malloc, realloc and free, which never fail,
 *	and might keep statistics on memory allocation...
 */

extern void *hmalloc(size_t size);
extern void *hcalloc(size_t nmemb, size_t size);
extern void *hrealloc(void *ptr, size_t size);
extern void hfree(void *ptr);

extern char *hstrdup(const char *s);

extern long int hmallocs;
extern long int hmallocs_b;
extern long int hcallocs;
extern long int hcallocs_b;
extern long int hfrees;
extern long int hfree_nulls;
extern long int hreallocs;
extern long int hstrdups;

#endif

