/* $LastChangedDate: 2024-03-25 02:17:19 +0100 (Mon, 25 Mar 2024) $ */
/*
 FimWindow.h : Fim's own windowing system header file

 (c) 2007-2024 Michele Martone

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
#ifndef FIM_WINDOW_H
#define FIM_WINDOW_H

#include "fim.h"
#include "DisplayDevice.h"

#ifdef FIM_WINDOWS

#include <vector>
#include <iostream>

#define FIM_DISABLE_WINDOW_SPLITTING 1

namespace fim
{
#define FIM_BUGGED_ENLARGE 0

/*
  The window class should model the behaviour of a binary splitting window
 in a portable manner.
  It should not be tied to a particular window system or graphical environment,
 but it should mimic the behaviour of Vim's windowing system.

 (x,y) : upper left point in
 +--------------+
 |              |
 |              |
 |              |
 +--------------+
 |              |
 +--------------+
                 (x+w,y+h) : lower right point out
*/
class Rect  FIM_FINAL
{
	public:
	fim_coo_t x_,y_,w_,h_;	// units, not pixels
#if !FIM_DISABLE_WINDOW_SPLITTING
	void print(void);
#endif

	Rect(fim_coo_t x,fim_coo_t y,fim_coo_t w,fim_coo_t h);

	Rect(const Rect& rect);

	public:

	enum Splitmode{ Left,Right,Upper,Lower};

#if !FIM_DISABLE_WINDOW_SPLITTING
	Rect hsplit(Splitmode s);
	Rect vsplit(Splitmode s);
	Rect split(Splitmode s);
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_err_t vlgrow(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t vlshrink(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t vugrow(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t vushrink(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);

	fim_err_t hlgrow(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t hrshrink(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t hrgrow(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	fim_err_t hlshrink(fim_coo_t units=FIM_CNS_WGROW_STEPS_DEFAULT);
	bool operator==(const Rect&rect)const;
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_coo_t setwidth(fim_coo_t w);
	fim_coo_t setheight(fim_coo_t h);
	fim_coo_t setxorigin(fim_coo_t x);
	fim_coo_t setyorigin(fim_coo_t y);
#endif
	fim_coo_t height(void)const;
	fim_coo_t width(void)const;
	fim_coo_t xorigin(void)const;
	fim_coo_t yorigin(void)const;
};

#ifdef FIM_NAMESPACES
class FimWindow FIM_FINAL:public Namespace
#else /* FIM_NAMESPACES */
class FimWindow FIM_FINAL
#endif /* FIM_NAMESPACES */
{
	private:

	enum Spacings{ hspacing=0, vspacing=0};
	enum Moves{Up,Down,Left,Right,NoMove};

	DisplayDevice *displaydevice_;
	public:
	Rect corners_;//,status,canvas;
	private:
	bool focus_;	// if 0 left/up ; otherwise right/lower

	FimWindow *first_,*second_;
	bool amroot_;
	
#if !FIM_DISABLE_WINDOW_SPLITTING
	void split(void);
	void hsplit(void);
	void vsplit(void);
	bool close(void);
	bool swap(void); // new
	void balance(void);
#endif
#if !FIM_DISABLE_WINDOW_SPLITTING
	bool chfocus(void);
	Moves move_focus(Moves move);
#endif
#if !FIM_DISABLE_WINDOW_SPLITTING
	Moves reverseMove(Moves move);
	bool normalize(void);
#endif
#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_err_t enlarge(fim_coo_t units);
	fim_err_t henlarge(fim_coo_t units);
	fim_err_t venlarge(fim_coo_t units);
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	bool can_vgrow(const FimWindow& window, fim_coo_t howmuch)const;
	bool can_hgrow(const FimWindow& window, fim_coo_t howmuch)const;
#endif

	private:
#if !FIM_DISABLE_WINDOW_SPLITTING
	explicit FimWindow(const FimWindow& root);
	bool isleaf(void)const;
	bool isvalid(void)const;
	bool issplit(void)const;
	bool ishsplit(void)const;
#endif
	bool isvsplit(void)const;
#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_err_t hnormalize(fim_coo_t x, fim_coo_t w);
	fim_err_t vnormalize(fim_coo_t y, fim_coo_t h);
#endif
#if !FIM_DISABLE_WINDOW_SPLITTING
	int count_hdivs(void)const;
	int count_vdivs(void)const;
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_err_t vlgrow(fim_coo_t units);
	fim_err_t vugrow(fim_coo_t units);
	fim_err_t vushrink(fim_coo_t units);
	fim_err_t vlshrink(fim_coo_t units);

	fim_err_t hlgrow(fim_coo_t units);
	fim_err_t hrgrow(fim_coo_t units);
	fim_err_t hlshrink(fim_coo_t units);
	fim_err_t hrshrink(fim_coo_t units);
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	FimWindow& focused(void)const;
	FimWindow& shadowed(void)const;
#endif

#if !FIM_DISABLE_WINDOW_SPLITTING
	FimWindow& upper(void);
	FimWindow& lower(void);
	FimWindow& left(void);
	FimWindow& right(void);

	bool operator==(const FimWindow&window)const;
#endif

	Viewport *viewport_;

#if !FIM_DISABLE_WINDOW_SPLITTING
	const FimWindow& c_focused(void)const;
	const FimWindow& c_shadowed(void)const;

	Viewport& current_viewport(void)const;
#endif
	CommandConsole& commandConsole_;

#if !FIM_DISABLE_WINDOW_SPLITTING
	FimWindow& operator= (const FimWindow& w);
#endif

	public:
	void setroot(void);	// only one root window should exist

	/* The only public member function launching exceptions is the constructor now.
	 * */
	FimWindow(CommandConsole& c, DisplayDevice *dd, const Rect& corners, Viewport* vp=FIM_NULL); // throws FIM_E_NO_MEM exception
	fim_err_t update(const Rect& corners);

	Viewport * current_viewportp(void)const;
        fim_cxr fcmd_cmd(const args_t&args);
	bool recursive_redisplay(void)const;	//exception safe
#if !FIM_DISABLE_WINDOW_SPLITTING
	bool recursive_display(void)const;		//exception safe
#endif

	const Image* getImage(void)const;		//exception safe
#if 0
	void print(void);
	void print_focused(void);
	void draw(void)const;
#endif

	~FimWindow(void) FIM_OVERRIDE;
	virtual size_t byte_size(void)const FIM_OVERRIDE;
	void should_redraw(enum fim_redraw_t sr = FIM_REDRAW_NECESSARY); /* FIXME: this is a wrapper to Viewport's, until multiple windows get introduced again. */
#if !FIM_DISABLE_WINDOW_SPLITTING
	fim_bool_t need_redraw(void)const;
#endif
};
} /* namespace fim */
#endif /* FIM_WINDOWS */
#endif /* FIM_WINDOW_H */
