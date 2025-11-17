/*
 SDLDevice.h : GTK device Fim driver header file

 (c) 2023-2024 Michele Martone

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

// TODO: this is work in progress, don't use it.
#ifndef FIM_GTKDEVICE_H
#define FIM_GTKDEVICE_H
#ifdef FIM_WITH_LIBGTK

class GTKDevice : public DisplayDevice {
	public:
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	GTKDevice(MiniConsole& mc_, fim::string opts);
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	GTKDevice(fim::string opts);
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	fim_err_t initialize(fim::sym_keys_t&)FIM_OVERRIDE;
	virtual fim_err_t reinit(const fim_char_t *rs) FIM_OVERRIDE;
	void finalize(void)FIM_OVERRIDE;
	fim_err_t display(const void*, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_coo_t, fim_flags_t)FIM_OVERRIDE;
	fim_coo_t get_chars_per_line() const FIM_OVERRIDE;
	fim_coo_t get_chars_per_column() const FIM_OVERRIDE;
	fim_coo_t width() const FIM_OVERRIDE;
	fim_coo_t height() const FIM_OVERRIDE;
	fim_bpp_t get_bpp() const FIM_OVERRIDE;
	virtual fim_coo_t status_line_height(void)const FIM_OVERRIDE;
	fim_err_t status_line(const fim_char_t*) FIM_OVERRIDE;
	fim_bool_t handle_console_switch()FIM_OVERRIDE{return false;}
	fim_err_t clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2) FIM_NOEXCEPT FIM_OVERRIDE;
	fim_err_t fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color) FIM_NOEXCEPT FIM_OVERRIDE;
	fim_err_t fs_puts(struct fs_font *f_, fim_coo_t x, fim_coo_t y, const fim_char_t *str) FIM_NOEXCEPT FIM_OVERRIDE ;
	fim_sys_int get_input(fim_key_t * c, bool want_poll=false)FIM_OVERRIDE;
	void fs_render_fb(fim_coo_t x, fim_coo_t y, FSXCharInfo *charInfo, fim_byte_t *data) FIM_NOEXCEPT;
	fim_key_t catchInteractiveCommand(fim_ts_t seconds)const FIM_OVERRIDE;
	virtual fim_err_t set_wm_caption(const fim_char_t *msg) FIM_OVERRIDE;
	fim_err_t menu_ctl(const char action, const fim_char_t *menuspec) FIM_OVERRIDE;
	static const fim_coo_t border_height_=1;
	static const int Bpp_{3};
	fim::string opts_;
private:
	inline void setpixel(fim_byte_t* rgb, fim_coo_t x, fim_coo_t y, fim_byte_t r, fim_byte_t g, fim_byte_t b);
	virtual fim_err_t resize(fim_coo_t w, fim_coo_t h) FIM_OVERRIDE ;
};
#endif /* FIM_WITH_LIBGTK */
#endif /* FIM_GTKDEVICE_H */
