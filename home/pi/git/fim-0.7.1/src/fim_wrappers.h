/* $LastChangedDate: 2022-11-05 13:20:39 +0100 (Sat, 05 Nov 2022) $ */
/*
 fim_wrappers.h : Some wrappers

 (c) 2011-2022 Michele Martone

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef FIM_WRAPPERS_H
#define FIM_WRAPPERS_H

#if FIM_WITH_DEBUG 
#include <map>
#include <cassert>
#endif

namespace fim
{
/* symbolic wrappers for memory handling calls */
#if FIM_WITH_DEBUG 
#define FIM_MEMDBG_EXT_DECLS extern std::map<void*,size_t> g_allocs; extern ssize_t g_allocs_n; extern ssize_t g_allocs_bytes;
#define FIM_MEMDBG_VERBOSE 0
inline void * fim_calloc_debug(size_t x,size_t y)
{
	FIM_MEMDBG_EXT_DECLS
       	void *p=std::calloc((x),(y)); 
	if (p)
	{
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << " [" << p << "] ";
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << " [" << (x*y) << "] ";
			std::cout << " -> ";
		}
		g_allocs[p] = (x*y);
		g_allocs_n++;
		g_allocs_bytes += (x*y);
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << std::endl;
		}
	}
	return p;
}
#define fim_calloc(x,y) fim_calloc_debug(x,y) 
#define fim_malloc(x) fim_calloc_debug(x,1)
inline void * fim_free_debug(void * p)
{ 
	FIM_MEMDBG_EXT_DECLS
	if(!p)
		return p;
	if(p)
	{
		if (g_allocs.find(p) == g_allocs.end())
		{
			std::cout << " [" << p << "] ";
			std::cout << " Freeing what never allocated ?!\n";
			assert(0);
		}
		if (g_allocs_n == 0)
		{
			std::cout << " Freeing more than allocated ?!\n";
		}
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << " [" << p << "] ";
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << " [" << g_allocs[p] << "] ";
			std::cout << " -> ";
		}
		g_allocs_n--;
		g_allocs_bytes -= g_allocs[p];
		g_allocs.erase(p);
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << std::endl;
		}
	}
	std::free(p);
	return p;
}
inline void * fim_realloc_debug(void * p, size_t sz)
{ 
	FIM_MEMDBG_EXT_DECLS
	if(!p)
		return fim_malloc(sz);
	if(p)
	{
		ssize_t osz = g_allocs[p];
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << " [" << p << "] ";
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << " [" << g_allocs[p] << "] ";
			std::cout << " -> +";
			std::cout << sz-osz;
			std::cout << " -> ";
		}
		g_allocs_bytes += sz - osz;
		g_allocs.erase(p);
		p = realloc(p, sz);
		g_allocs[p]=sz;
		if (FIM_MEMDBG_VERBOSE)
		{
			std::cout << g_allocs_n << "/" << g_allocs_bytes;
			std::cout << std::endl;
		}
	}
	return p;
}
#define fim_free(x) fim_free_debug(x), x=FIM_NULL
#define fim_stralloc(x) (fim_char_t*) fim_calloc((x),(1)) /* ensures that first char is NUL */
#define fim_realloc(p,sz) fim_realloc_debug(p,sz)
#else /* FIM_WITH_DEBUG */
#define fim_calloc(x,y) std::calloc((x),(y)) /* may make this routine aligned in the future */
#define fim_malloc(x) std::malloc(x)
#define fim_realloc(p,sz) realloc(p,sz)
#define fim_free(x) std::free(x), x=FIM_NULL
#define fim_stralloc(x) (fim_char_t*) std::calloc((x),(1)) /* ensures that first char is NUL */
#endif /* FIM_WITH_DEBUG */
#define fim_memset(x,y,z) std::memset(x,y,z)
#define fim_bzero(x,y) fim_memset(x,0,y) /* bzero has been made legacy by POSIX.2001 and deprecated since POSIX.2004 */
}
#endif /* FIM_WRAPPERS_H */
