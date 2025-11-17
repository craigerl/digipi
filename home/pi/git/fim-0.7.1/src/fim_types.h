/* $LastChangedDate: 2024-02-07 23:35:22 +0100 (Wed, 07 Feb 2024) $ */
/*
 fim_types.h : Basic Fim type declarations

 (c) 2011-2024 Michele Martone

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

#ifndef FIM_TYPES_FIM_H
#define FIM_TYPES_FIM_H

#if	defined(__GNUC__)
#define FIM_RSTRCT __restrict__
#else	/* defined(__GNUC__) */
#define FIM_RSTRCT
#endif	/* defined(__GNUC__) */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif /* HAVE_CONFIG_H */
#if FIM_WANT_LONG_INT
#include <stdint.h>
#endif /* FIM_WANT_LONG_INT */

#if FIM_WANT_LONG_INT
	typedef int64_t fim_int;	/* a type for fim's internal integer type, always signed */
#define FIM_LARGE_FIM_INT 1		/* == (sizeof(fim_int)!=sizeof(int)) */
#else /* FIM_WANT_LONG_INT */
	typedef int fim_int;		/* a type for fim's internal integer type, always signed */
#define FIM_LARGE_FIM_INT 0		/* == (sizeof(fim_int)!=sizeof(int)) */
#endif /* FIM_WANT_LONG_INT */
	typedef unsigned int fim_uint;	/* unsigned int */
	typedef int fim_pan_t;		/* a type for pixel offsets (neg/pos)  */
	typedef int fim_off_t;		/* a type for pixel offsets (positive)  */
	typedef float fim_scale_t;	/* a type for image scaling */
	typedef float fim_angle_t;	/* a type for angles */
	typedef float fim_float_t;	/* a type for floats */
	typedef fim_int   fim_page_t;	/* a type for multipage document pages */
	typedef int   fim_pgor_t;	/* a type for page orientation */
	typedef bool   fim_bool_t;	/* a type for bolean expressions */
	typedef int fim_coo_t;		/* a type for coordinates */
	typedef int fim_cc_t;		/* a type for console control */
	typedef int fim_flags_t;	/* a type for display flags */
	typedef int fim_bpp_t;		/* a type for bits Per Pixel */
	typedef int fim_key_t;		/* a type for keycodes */
	typedef int fim_err_t;		/* a type for errors */
	typedef int fim_perr_t;		/* a type for program errors */
	typedef int fim_status_t;	/* a type for fim's status */
	typedef int fim_cycles_t;	/* a type for fim's cycles */
	typedef int fim_cmd_type_t;	/* a type for fim's command types */
	typedef int fim_var_t;		/* a type for fim's variable types */
	typedef int fim_str_t;		/* a type for stdin/stdout streams */
	typedef int fim_sys_int;	/* always int */
	typedef unsigned int fim_color_t;	/* >= 4 bytes */

	typedef float fim_ts_t;		/* a type for time, in seconds */
	typedef int fim_tms_t;		/* a type for time, in milliseconds */
	typedef double fim_fms_t;		/* a type for time, in milliseconds, floating point */
	typedef unsigned long fim_tus_t;	/* a type for time, in microseconds */
	typedef char fim_char_t;	/* a type for chars */
	typedef unsigned char fim_byte_t;	/* a type for bytes */
	typedef size_t fim_size_t;	/* always size_t */
	typedef size_t fim_pxc_t;	/* pixel count */
	enum fim_redraw_t { FIM_REDRAW_UNNECESSARY=0, FIM_REDRAW_NECESSARY=1};
	// TODO: move using aliases here ...
	// TODO: same also in CommandConsole.h

/* we wait for variadic macros support in standard C++ */
#define FIM_FPRINTF fprintf

#define FIM_NULL nullptr
#define FIM_ENUM_BASE : fim_flags_t /* In the future might want to add the class attribute to make the enums scoped. */

#endif /* FIM_TYPES_FIM_H */
