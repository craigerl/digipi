/* $LastChangedDate: 2024-03-22 01:13:38 +0100 (Fri, 22 Mar 2024) $ */
/*
 CACADevice.h : libcaca device Fim driver header file

 (c) 2008-2024 Michele Martone

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
#ifndef FIM_CACADEVICE_H
#define FIM_CACADEVICE_H
#ifdef FIM_WITH_LIBCACA

#include "DisplayDevice.h"

#define FIM_WANTS_CACA_VERSION 1
#if ( FIM_WANTS_CACA_VERSION == 0 )
#include <caca0.h>
#else
#include <caca.h>
#endif

#define FIM_LIBCACA_FONT_HEIGHT 1
class CACADevice FIM_FINAL:public DisplayDevice 
{
	private:
	fim::string dopts_;

#if ( FIM_WANTS_CACA_VERSION == 0 )
	unsigned int r_[256], g_[256], b_[256], a_[256];
	int XSIZ_, YSIZ_;
	struct caca_bitmap *caca_bitmap_;
	fim_char_t *bitmap_;
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
	caca_canvas_t *cv_; // {} pending
	caca_display_t *dp_; // {} pending
#endif
	public:
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	CACADevice(MiniConsole& mc_, fim::string dopts):DisplayDevice(mc_),dopts_(dopts)
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	CACADevice(fim::string dopts):DisplayDevice(),dopts_(dopts)
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	{
#if ( FIM_WANTS_CACA_VERSION == 1 )
		// use default initializers as soon as C++03 is phased out
		cv_=FIM_NULL;
		dp_=FIM_NULL;
#endif /* FIM_WANTS_CACA_VERSION */
	}

	fim_err_t display(
		const void *ida_image_img, // source image structure (struct ida_image *)(but we refuse to include header files here!)
		//void* rgb,// destination gray array and source rgb array
		fim_coo_t iroff,fim_coo_t icoff, // row and column offset of the first input pixel
		fim_coo_t irows,fim_coo_t icols,// rows and columns in the input image
		fim_coo_t icskip,	// input columns to skip for each line
		fim_coo_t oroff,fim_coo_t ocoff,// row and column offset of the first output pixel
		fim_coo_t orows,fim_coo_t ocols,// rows and columns to draw in output buffer
		fim_coo_t ocskip,// output columns to skip for each line
		fim_flags_t flags// some flags
		) FIM_OVERRIDE;
	const char * get_dither_mode(void)const;
	int initialize(sym_keys_t &sym_keys) FIM_OVERRIDE;
	void finalize(void) FIM_OVERRIDE;

	fim_coo_t get_chars_per_line(void)const FIM_OVERRIDE; 
	int txt_width(void)const;
	int txt_height(void)const;
	int width(void)const FIM_OVERRIDE;
	int height(void)const FIM_OVERRIDE;
	fim_err_t status_line(const fim_char_t *msg) FIM_OVERRIDE;
	void status_screen(int desc,int draw_output){}
	fim_err_t console_control(fim_cc_t code);
	fim_bool_t handle_console_switch(void) FIM_OVERRIDE;
	fim_err_t clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2) FIM_OVERRIDE;
	fim_err_t fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color) FIM_OVERRIDE {/* note: bogus implementation */ return clear_rect(x1,x2,y1,y2); }
	fim_err_t fs_puts(struct fs_font *f, fim_coo_t x, fim_coo_t y, const fim_char_t *str) FIM_OVERRIDE;

	fim_coo_t get_chars_per_column(void)const FIM_OVERRIDE;
	fim_bpp_t get_bpp(void)const FIM_OVERRIDE
	{
		return 1;
	}
	virtual fim_coo_t status_line_height(void)const FIM_OVERRIDE;
	virtual fim_coo_t font_height(void)const FIM_OVERRIDE;
	fim_sys_int get_input(fim_key_t * c, bool want_poll=false) FIM_OVERRIDE;
	fim_err_t resize(fim_coo_t w, fim_coo_t h) FIM_OVERRIDE;
	virtual fim_err_t set_wm_caption(const fim_char_t *msg) FIM_OVERRIDE;
	virtual fim_err_t reinit(const fim_char_t *rs) FIM_OVERRIDE;
};
#endif /* FIM_WITH_LIBCACA */
#endif /* FIM_CACADEVICE_H */
