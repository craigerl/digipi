/* $LastChangedDate: 2024-04-02 13:27:55 +0200 (Tue, 02 Apr 2024) $ */
/*
 SDLDevice.h : sdllib device Fim driver header file

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
#ifndef FIM_SDLDEVICE_H
#define FIM_SDLDEVICE_H
#ifdef FIM_WITH_LIBSDL

#define FIM_WITH_LIBSDL_VERSION FIM_WITH_LIBSDL

#include "DisplayDevice.h"
#include <SDL.h>
#if HAVE_EMSCRIPTEN_H 
#include <emscripten.h>
#endif /* HAVE_EMSCRIPTEN_H  */
#if (FIM_WITH_LIBSDL_VERSION == 2)
#include <SDL_video.h>
#endif

class SDLDevice FIM_FINAL :public DisplayDevice 
{
private:

	SDL_Surface *screen_;
#if (FIM_WITH_LIBSDL_VERSION == 1)
#else
	SDL_Texture *texture_;
#endif
	SDL_Event event_;
#if (FIM_WITH_LIBSDL_VERSION == 1)
	const SDL_VideoInfo* vi_;
	SDL_VideoInfo bvi_;
#else
	SDL_Window* wi_;
	SDL_Renderer *re_;
#endif

	fim_coo_t current_w_;
	fim_coo_t current_h_;
	fim_bpp_t Bpp_,bpp_;
	fim::string opts_;
	static const fim_coo_t border_height_=1;
	bool want_windowed_;
	bool want_mouse_display_;
	bool want_resize_;
#if (FIM_WITH_LIBSDL_VERSION == 1)
	SDL_Rect ** modes_;
#else
	int numDisplayModes_;
#endif

	void get_modes_list(void);
	fim_err_t get_resolution(const char spec, fim_coo_t & w, fim_coo_t & h) const;
public:
#if (FIM_WITH_LIBSDL_VERSION == 2)
	fim_sys_int get_input_inner(fim_key_t * c, SDL_Event*eventp, fim_sys_int *keypressp, bool want_poll) const;
#endif

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	SDLDevice(MiniConsole& mc_, fim::string opts);
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	SDLDevice( fim::string opts);
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */

	virtual fim_err_t  display(
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

	fim_err_t initialize(sym_keys_t &sym_keys)FIM_OVERRIDE;
	void finalize(void) FIM_OVERRIDE;

	fim_coo_t get_chars_per_line(void)const FIM_OVERRIDE;
	fim_coo_t get_chars_per_column(void)const FIM_OVERRIDE;
	fim_coo_t width(void)const FIM_OVERRIDE;
	fim_coo_t height(void)const FIM_OVERRIDE;
	fim_err_t status_line(const fim_char_t *msg) FIM_OVERRIDE;
	fim_bool_t handle_console_switch(void) FIM_OVERRIDE { return false; }
	fim_err_t clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2) FIM_NOEXCEPT FIM_OVERRIDE;
	fim_sys_int get_input(fim_key_t * c, bool want_poll=false) FIM_OVERRIDE;
	virtual fim_key_t catchInteractiveCommand(fim_ts_t seconds)const FIM_OVERRIDE;
	void fs_render_fb(fim_coo_t x, fim_coo_t y, FSXCharInfo *charInfo, fim_byte_t *data) FIM_NOEXCEPT;
	fim_err_t fs_puts(struct fs_font *f_, fim_coo_t x, fim_coo_t y, const fim_char_t *str) FIM_NOEXCEPT FIM_OVERRIDE ;
	void flush(void)FIM_OVERRIDE;
	void lock(void) FIM_OVERRIDE ;
	void unlock(void) FIM_OVERRIDE ;
	fim_bpp_t get_bpp(void)const FIM_OVERRIDE { return bpp_; }
	bool sdl_window_update(void);
	virtual fim_coo_t status_line_height(void)const FIM_OVERRIDE;
private:
	inline void setpixel(SDL_Surface *screen_, fim_coo_t x, fim_coo_t y, Uint8 r, Uint8 g, Uint8 b) FIM_NOEXCEPT;
	void status_screen_(int desc,int draw_output){ return ; }
	fim_err_t fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color) FIM_NOEXCEPT FIM_OVERRIDE;
	fim_coo_t txt_width(void)const ;
	fim_coo_t txt_height(void)const ;
	virtual fim_err_t resize(fim_coo_t w, fim_coo_t h) FIM_OVERRIDE ;
	bool allowed_resolution(fim_coo_t w, fim_coo_t h) const;
	virtual fim_err_t reinit(const fim_char_t *rs) FIM_OVERRIDE;
	fim_err_t parse_optstring(const fim_char_t *os);
	virtual fim_err_t set_wm_caption(const fim_char_t *msg) FIM_OVERRIDE;
	fim_err_t reset_wm_caption(void)const;
	fim_err_t post_wmresize(void);
#if (FIM_WITH_LIBSDL_VERSION == 2)
	void sdl2_redraw(void)const;
#endif
};

#endif /* FIM_WITH_LIBSDL */
#endif /* FIM_SDLDEVICE_H */
