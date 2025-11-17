/* $LastChangedDate: 2013-07-04 23:07:07 +0200 (Thu, 04 Jul 2013) $ */
/*
 fim_limits.h : Basic Fim types limits

 (c) 2011-2013 Michele Martone

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

#ifndef FIM_LIMITS_FIM_H
#define FIM_LIMITS_FIM_H

namespace fim
{
#define FIM_IS_SIGNED(T)   (((T)0) > (((T)-1)))
#define FIM_PROBABLY_SAME_TYPES(T1,T2) ((FIM_IS_SIGNED(T1)==FIM_IS_SIGNED(T2)) && sizeof(T1)==sizeof(T2))
#define FIM_IS_UNSIGNED(T) (!FIM_IS_SIGNED(T))
#define FIM_MAX_UNSIGNED(T) ((T)-1)
#define FIM_MAX_SIGNED(T) (FIM_HALF_MAX_SIGNED(T) - 1 + FIM_HALF_MAX_SIGNED(T))
#define FIM_MIN_SIGNED(T) (-1 - FIM_MAX_SIGNED(T))
#define FIM_MAX_VALUE_FOR_TYPE(T) (FIM_IS_SIGNED(T)?FIM_MAX_SIGNED(T):FIM_MAX_UNSIGNED(T))
#define FIM_CHAR_BITS (8)
#define FIM_BYTES_COUNT (1<<(sizeof(T)*FIM_CHAR_BITS))
}
#endif /* FIM_LIMITS_FIM_H */
