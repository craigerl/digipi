/* $LastChangedDate: 2024-03-25 02:17:19 +0100 (Mon, 25 Mar 2024) $ */
/*
 Benchmarkable.h.h : header file for benchmarkable classes

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
#ifndef FIM_BENCHMARKABLE_H
#define FIM_BENCHMARKABLE_H
#include "fim.h"
#if FIM_WANT_BENCHMARKS
	class Benchmarkable{
		private:
		// fim_int benchmarkstate{0};
	       	public:
		virtual fim_int get_n_qbenchmarks(void)const=0;
		virtual void quickbench(fim_int qbi)=0;
		virtual string get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const=0;
		virtual void quickbench_init(fim_int qbi)=0;
		virtual void quickbench_finalize(fim_int qbi)=0;
       	};
#endif /* FIM_WANT_BENCHMARKS */
#endif /* FIM_BENCHMARKABLE_H */
