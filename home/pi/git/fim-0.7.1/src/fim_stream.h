/* $LastChangedDate: 2022-10-22 00:37:29 +0200 (Sat, 22 Oct 2022) $ */
/*
 fim_stream.h : Textual output facility

 (c) 2007-2022 Michele Martone

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
#ifndef FIM_FIM_STREAM_H
#define FIM_FIM_STREAM_H
#include "fim.h"
namespace fim
{
	/*
	 * this class is a stream used in Fim and should output to an internal console.
	 *
	 * TODO: error and to file dump. maybe, some day.
	 *	 move here the console handling functionalities.
	 * TODO: introduce an enum for output selection
	 * */

	class fim_stream
	{
		fim_str_t fd_;
		public:
		fim_stream(fim_str_t fd=-1);

		fim_stream& operator<<(const  fim_char_t* s);

		fim_stream& operator<<(const  fim_char_t c);

		fim_stream& operator<<(const fim_byte_t* s);

		fim_stream& operator<<(const  fim::string&s);

		fim_stream& operator<<(float f);
#if FIM_WANT_LONG_INT
		fim_stream& operator<<(fim_int i);
#endif /* FIM_WANT_LONG_INT */

		fim_stream& operator<<(int i);
		fim_stream& operator<<(unsigned int i);
	};
}
#endif /* FIM_FIM_STREAM_H */
