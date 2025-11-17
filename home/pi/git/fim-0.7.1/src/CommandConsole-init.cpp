/* $LastChangedDate: 2024-04-21 23:09:14 +0200 (Sun, 21 Apr 2024) $ */
/*
 CommandConsole-init.cpp : Fim console initialization

 (c) 2010-2024 Michele Martone

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
#include "fim.h"
#ifdef FIM_DEFAULT_CONFIGURATION
#include "conf.h" /* FIM_DEFAULT_CONFIG_FILE_CONTENTS declared here */ 
#endif /* FIM_DEFAULT_CONFIGURATION */
#include <sys/time.h>
#include <errno.h>

#ifdef FIM_USE_READLINE
#include "readline.h"
#endif /* FIM_USE_READLINE */

#include <type_traits>

namespace fim
{

#if FIM_WANT_BENCHMARKS
static fim_err_t fim_bench_subsystem(Benchmarkable * bo)
{
	fim_int qbn,qbi;
	fim_fms_t tbtime,btime;// ms
	size_t times=1;

	if(!bo)
		return FIM_ERR_GENERIC;
	qbn=bo->get_n_qbenchmarks();

	for(qbi=0;qbi<qbn;++qbi)
	{
		times=1;
		tbtime=1000.0,btime=0.0;// ms
		bo->quickbench_init(qbi);
		do
		{
			btime=-getmilliseconds();
#if 0 
			fim_bench_video(NULL);
#endif /* 0 */
			bo->quickbench(qbi);
			btime+=getmilliseconds();
			++times;
			tbtime-=btime;
		}
		while(btime>=0.0 && tbtime>0.0 && times>0);
		--times;
		tbtime=1000.0-tbtime;
		std::cout << bo->get_bresults_string(qbi,times,tbtime);
		bo->quickbench_finalize(qbi);
	}
	return FIM_ERR_NO_ERROR;
}
#endif /* FIM_WANT_BENCHMARKS */

	fim_err_t CommandConsole::init(fim::string device)
	{
		/*
		 * TODO: move most of this stuff to the constructor, some day.
		 */
		fim_int xres=0,yres=0;
		bool device_failure=false;
		int dosso=device.find(FIM_SYM_DEVOPTS_SEP_STR);
#ifdef FIM_USE_READLINE
		bool wcs = false; /* want cookie stream (readline but no stdin) */
#endif /* FIM_USE_READLINE */
		std::string dopts;
		/* prevents atof, sprintf and such conversion mismatches. */
		setlocale(LC_ALL,"C");	/* portable (among Linux hosts): should use dots for numerical radix separator */
		//setlocale(LC_NUMERIC,"en_US"); /* lame  */
		//setlocale(LC_ALL,""); /* just lame */
		assert(displaydevice_==FIM_NULL);

		if(dosso>0)
			dopts=device.substr(dosso+strlen(FIM_SYM_DEVOPTS_SEP_STR),device.length()).c_str();

#ifndef FIM_WITH_NO_FRAMEBUFFER
		if(device.find(FIM_DDN_INN_FB)==0)
		{
			extern fim_char_t *default_fbdev,*default_fbmode; /* FIXME: we don't want globals! */
			extern int default_vt;
			extern float default_fbgamma;
			FramebufferDevice * ffdp=FIM_NULL;

			displaydevice_=new FramebufferDevice(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
					mc_
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
					);
			const bool do_boz_patch = (dopts.size() && dopts[0]=='S')?0:(0xbabebabe==0xbabebabe);
			if( displaydevice_)
			{
				ffdp=((FramebufferDevice*)displaydevice_);
				if(default_fbdev)ffdp->set_fbdev(default_fbdev);
				if(default_fbmode)ffdp->set_fbmode(default_fbmode);
				if(default_vt!=-1)ffdp->set_default_vt(default_vt);
				if(default_fbgamma!=-1.0)ffdp->set_default_fbgamma(default_fbgamma);
				ffdp=NULL;
			}
			if(!displaydevice_ || ((FramebufferDevice*)displaydevice_)->framebuffer_init(do_boz_patch))
			{
				if(do_boz_patch) // if not, allow for reinit
					cleanup();
				displaydevice_=NULL;
				if(ffdp)
					delete ffdp;
				return FIM_ERR_GENERIC;
			}
			ffdp=((FramebufferDevice*)displaydevice_);
			mangle_tcattr_=true;
		}
#endif	//#ifndef FIM_WITH_NO_FRAMEBUFFER


		#ifdef FIM_WITH_LIBIMLIB2
		if(device.find(FIM_DDN_INN_IL2)==0)
		{
			DisplayDevice *imld=FIM_NULL;
			imld=new Imlib2Device(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
					mc_,
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
					dopts
					);
			if(imld && imld->initialize(sym_keys_)!=FIM_ERR_NO_ERROR){delete imld ; imld=FIM_NULL;}
			if(imld && displaydevice_==FIM_NULL)
			{
				displaydevice_=imld;
				mangle_tcattr_=false;
			}
			else
			{
				device_failure=true;
			}
#ifdef FIM_USE_READLINE
			wcs = true;
#endif /* FIM_USE_READLINE */
		}
		#endif /* FIM_WITH_LIBIMLIB2 */

		#ifdef FIM_WITH_LIBSDL
		if(device.find(FIM_DDN_INN_SDL)==0)
		{
			DisplayDevice *sdld=FIM_NULL;
			fim::string fopts;
#if FIM_WANT_SDL_OPTIONS_STRING 
			fopts=dopts;
#endif /* FIM_WANT_SDL_OPTIONS_STRING */
			sdld=new SDLDevice(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
					mc_,
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
					fopts
					);
		       	if(sdld && sdld->initialize(sym_keys_)!=FIM_ERR_NO_ERROR){delete sdld ; sdld=FIM_NULL;}
			if(sdld && displaydevice_==FIM_NULL)
			{
				displaydevice_=sdld;
				mangle_tcattr_=false;
			}
			else
			{
				device_failure=true;
			}
#ifdef FIM_USE_READLINE
			wcs = true;
#endif /* FIM_USE_READLINE */
		}
		#endif /* FIM_WITH_LIBSDL */

		#ifdef FIM_WITH_LIBGTK
		if(device.find(FIM_DDN_INN_GTK)==0)
		{
			DisplayDevice *gtkd=FIM_NULL;
			gtkd=new GTKDevice(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
					mc_,
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
					dopts
					);
			if(gtkd && gtkd->initialize(sym_keys_)!=FIM_ERR_NO_ERROR){delete gtkd ; gtkd=FIM_NULL;}
			if(gtkd && displaydevice_==FIM_NULL)
			{
				displaydevice_=gtkd;
				mangle_tcattr_=false;
			}
			else
				device_failure=true;
		}
		#endif /* FIM_WITH_LIBGTK */

		#ifdef FIM_WITH_LIBCACA
		if(device.find(FIM_DDN_INN_CACA)==0)
		{
			DisplayDevice *cacad=FIM_NULL;
			cacad=new CACADevice(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
					mc_,
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
					dopts
					);
			if(cacad && cacad->initialize(sym_keys_)!=FIM_ERR_NO_ERROR){delete cacad ; cacad=FIM_NULL;}
			if(cacad && displaydevice_==FIM_NULL)
			{
				displaydevice_=cacad;
				mangle_tcattr_=false;
			}
			else
				device_failure=true;
		}
		#endif /* FIM_WITH_LIBCACA */

		#ifdef FIM_WITH_AALIB
		if(device.find(FIM_DDN_INN_AA)==0)
		{
		aad_=new AADevice(
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
				mc_,
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
				dopts
				);

		if(aad_ && aad_->initialize(sym_keys_)!=FIM_ERR_NO_ERROR){delete aad_ ; aad_=FIM_NULL;}
		{
		if(aad_ && displaydevice_==FIM_NULL)
		{
			displaydevice_=aad_;
#if FIM_WANT_SCREEN_KEY_REMAPPING_PATCH
			/*
			 * It seems like the keymaps get shifted when running under screen.
			 * Regard this as a patch.
			 * */
			const fim_char_t * const term = fim_getenv(FIM_CNS_TERM_VAR);
			if(term && string(term).re_match("screen"))
			{
				sym_keys_[FIM_KBD_LEFT]-=3072;
				sym_keys_[FIM_KBD_RIGHT]-=3072;
				sym_keys_[FIM_KBD_UP]-=3072;
				sym_keys_[FIM_KBD_DOWN]-=3072;
			}
			mangle_tcattr_=false;
#endif /* FIM_WANT_SCREEN_KEY_REMAPPING_PATCH */
		}
		else device_failure=true;
		}
		}
		#endif /* FIM_WITH_AALIB */
		if(mangle_tcattr_)
			tty_raw();// this inhibits unwanted key printout (raw mode), and saves the current tty state
		// FIXME: an error here on, leaves the terminal in raw mode, and this is not cool

		if( displaydevice_==FIM_NULL)
		{
			device=FIM_DDN_INN_DUMB;
			displaydevice_=&dummydisplaydevice_;
			if(device_failure)
			{
				std::cerr << "Failure using the \""<<device<<"\" display device driver string (wrong options to it ?)!\n";
				cleanup();
				return FIM_ERR_UNSUPPORTED_DEVICE;

			}
			else
			if(device!=FIM_DDN_INN_DUMB)
			{
				if( 0
		#ifndef FIM_WITH_AALIB
					|| device==FIM_DDN_INN_AA
		#endif /* FIM_WITH_AALIB */
		#ifndef FIM_WITH_LIBSDL
					|| device==FIM_DDN_INN_SDL
		#endif /* FIM_WITH_LIBSDL */
		#ifndef FIM_WITH_LIBGTK
					|| device==FIM_DDN_INN_GTK
		#endif /* FIM_WITH_LIBGTK */
		#ifdef FIM_WITH_NO_FRAMEBUFFER
					|| device==FIM_DDN_INN_FB
		#endif /* FIM_WITH_NO_FRAMEBUFFER */
		#ifndef FIM_WITH_LIBIMLIB2
					|| device==FIM_DDN_INN_IL2
		#endif /* FIM_WITH_LIBIMLIB2 */
		#ifndef FIM_WITH_LIBCACA
					|| device==FIM_DDN_INN_CACA
		#endif /* FIM_WITH_LIBCACA */
				)
					std::cerr << "Device string \""<<device<<"\" has been configured out (disabled) at build time.\n";
				else
					std::cerr << "Unrecognized display device string \""<<device<<"\".\n";
				std::cerr << "Valid choices are " << FIM_DDN_VARS_IN << "!\n";
			}
			std::cerr << "Using the default \""<<FIM_DDN_INN_DUMB<<"\" display device instead.\n";
		}

		setVariable(FIM_VID_DEVICE_DRIVER,device);
		mc_.setDisplayDevice(displaydevice_);
		xres=displaydevice_->width(),yres=displaydevice_->height();
		displaydevice_->format_console();

#ifdef FIM_WINDOWS
		/* true pixels if we are in framebuffer mode */
		/* fake pixels if we are in text (er.. less than!) mode */
		if( xres<=0 || yres<=0 )
		{
			std::cerr << "Unable to spawn a suitable display.\n";
		       	return FIM_ERR_BAD_PARAMS;
		}

		try
		{
			window_ = new FimWindow( *this, displaydevice_, Rect(0,0,xres,yres) );

			if(window_)window_->setroot();
		}
		catch(FimException e)
		{
			if( e == FIM_E_NO_MEM || true )
				quit(FIM_E_NO_MEM);
		}

		addCommand(Command(FIM_FLT_WINDOW,FIM_CMD_HELP_WINDOW, window_,&FimWindow::fcmd_cmd));
#else /* FIM_WINDOWS */
		try
		{
			viewport_ = new Viewport(*this, displaydevice_);
		}
		catch(FimException e)
		{
			if( e == FIM_E_NO_MEM || true ) quit(FIM_E_NO_MEM);
		}
#endif /* FIM_WINDOWS */
#ifdef FIM_NAMESPACES
		if(displaydevice_)
			setVariable(FIM_VID_FIM_BPP ,displaydevice_->get_bpp());
#endif /* FIM_NAMESPACES */
		setVariable(FIM_VID_SCREEN_WIDTH, xres);
		setVariable(FIM_VID_SCREEN_HEIGHT,yres);

		/* Here the program loads initialization scripts */

	    	FIM_AUTOCMD_EXEC(FIM_ACM_PRECONF,"");
	    	FIM_AUTOCMD_EXEC(FIM_ACM_PREHFIMRC,"");

  #ifndef FIM_WANT_NOSCRIPTING
		if(!preConfigCommand_.empty())
			execute_internal(preConfigCommand_.c_str(),FIM_X_HISTORY);

		if(getIntVariable(FIM_VID_NO_DEFAULT_CONFIGURATION)==0 )
		{
    #ifdef FIM_DEFAULT_CONFIGURATION
			/* so the user could inspect what goes in the default configuration */
			setVariable(FIM_VID_FIM_DEFAULT_CONFIG_FILE_CONTENTS,FIM_DEFAULT_CONFIG_FILE_CONTENTS);
			execute_internal(FIM_DEFAULT_CONFIG_FILE_CONTENTS,FIM_X_QUIET);
    #endif		/* FIM_DEFAULT_CONFIGURATION */
		}
	    	FIM_AUTOCMD_EXEC(FIM_ACM_POSTHFIMRC,""); 
  #endif /* FIM_WANT_NOSCRIPTING */

#ifndef FIM_NOFIMRC
  #ifndef FIM_WANT_NOSCRIPTING
		const std::string home_env_var = fim_getenv(FIM_CNS_HOME_VAR);

	    	FIM_AUTOCMD_EXEC(FIM_ACM_PREGFIMRC,""); 

		if((getIntVariable(FIM_VID_LOAD_DEFAULT_ETC_FIMRC)==1 )
		&& (getStringVariable(FIM_VID_DEFAULT_ETC_FIMRC)!=FIM_CNS_EMPTY_STRING)
				)
		{
			string ef=getStringVariable(FIM_VID_DEFAULT_ETC_FIMRC);
			if(is_file(ef.c_str()))
				if(FIM_ERR_NO_ERROR!=executeFile(ef.c_str()))
		    			std::cerr << "Problems loading " << ef << std::endl;
		}
		
		/* execution of command line-set autocommands */
	    	FIM_AUTOCMD_EXEC(FIM_ACM_POSTGFIMRC,""); 
	    	FIM_AUTOCMD_EXEC(FIM_ACM_PREUFIMRC,""); 
  #endif /* FIM_WANT_NOSCRIPTING */
#endif /* FIM_NOFIMRC */

		{
#ifndef FIM_WANT_NOSCRIPTING
			#include "grammar.h"
			setVariable(FIM_VID_FIM_DEFAULT_GRAMMAR_FILE_CONTENTS,FIM_DEFAULT_GRAMMAR_FILE_CONTENTS);
#endif /* FIM_WANT_NOSCRIPTING */
		}

#ifndef FIM_NOFIMRC
  #ifndef FIM_WANT_NOSCRIPTING
		if(home_env_var.size())//strlen("/" FIM_CNS_USR_RC_FILEPATH)+2
		{
			const std::string rcfile = home_env_var + "/" FIM_CNS_USR_RC_FILEPATH;
			if(getIntVariable(FIM_VID_NO_RC_FILE)!=1 )
			{
				if(
					(!is_file(rcfile) || FIM_ERR_NO_ERROR!=executeFile(rcfile.c_str()))
	//			&&	(!is_file(FIM_CNS_SYS_RC_FILEPATH) || FIM_ERR_NO_ERROR!=executeFile(FIM_CNS_SYS_RC_FILEPATH))
				  )
  #endif /* FIM_WANT_NOSCRIPTING */
#endif /* FIM_NOFIMRC */
				{
					/*
					 if no configuration file is present, or fails loading,
					 we use the default configuration (raccomended !)  !	*/
  #ifdef FIM_DEFAULT_CONFIGURATION
					// 20110529 commented the following, as it is a (harmful) duplicate execution 
					//execute_internal(FIM_DEFAULT_CONFIG_FILE_CONTENTS,FIM_X_QUIET);
  #endif		/* FIM_DEFAULT_CONFIGURATION */
				}
#ifndef FIM_NOFIMRC
  #ifndef FIM_WANT_NOSCRIPTING
			}
		}
  #endif		/* FIM_WANT_NOSCRIPTING */
#endif		/* FIM_NOFIMRC */
		setVariable(FIM_VID_ALL_FILE_LOADERS,Var(fim_loaders_to_string()));
	    	FIM_AUTOCMD_EXEC(FIM_ACM_POSTUFIMRC,""); 
	    	FIM_AUTOCMD_EXEC(FIM_ACM_POSTCONF,"");
#ifndef FIM_WANT_NOSCRIPTING
		for(size_t i=0;i<scripts_.size();++i)
			executeFile(scripts_[i].c_str());
#endif		/* FIM_WANT_NOSCRIPTING */
#ifdef FIM_AUTOCMDS
		if(!postInitCommand_.empty())
			autocmd_add(FIM_ACM_PREEXECUTIONCYCLE,"",postInitCommand_.c_str());
		if(!postExecutionCommand_.empty())
			autocmd_add(FIM_ACM_POSTEXECUTIONCYCLE,"",postExecutionCommand_.c_str());
#endif /* FIM_AUTOCMDS */

#ifdef FIM_USE_READLINE
		rl::initialize_readline( displaydevice_==FIM_NULL, wcs );
		load_or_save_history(true);
#endif /* FIM_USE_READLINE */

		static_assert(std::is_signed<fim_int>(),FIM_EMSG_ITE);
		static_assert(std::is_floating_point<fim_scale_t>(),FIM_EMSG_ITE);
		static_assert(std::is_floating_point<fim_angle_t>(),FIM_EMSG_ITE);
		static_assert(std::is_floating_point<fim_float_t>(),FIM_EMSG_ITE);
		static_assert(std::is_enum<fim_redraw_t>(),FIM_EMSG_ITE);
		static_assert(std::is_class<Var>(),FIM_EMSG_ITE);
		static_assert(std::is_abstract<DisplayDevice>(),FIM_EMSG_ITE);
		static_assert(std::is_polymorphic<DisplayDevice>(),FIM_EMSG_ITE);
		static_assert(std::is_polymorphic<Namespace>(),FIM_EMSG_ITE);
		static_assert(std::is_polymorphic<MiniConsole>(),FIM_EMSG_ITE);
		static_assert(*FIM_SYM_UNKNOWN_STRING==FIM_SYM_UNKNOWN_CHAR,FIM_EMSG_ICE);
		if(getIntVariable(FIM_VID_SANITY_CHECK)==1 )
		{
			fim_err_t errval = FIM_ERR_NO_ERROR;
			assert( fim::string("-file").re_match("^-"));
			assert( fim::string("file ").re_match(" "));
			assert(!fim::string("file").re_match(" "));
			assert(!fim::string("file").re_match("^-"));
			assert(!fim::string("-file").re_match(FIM_CNS_PIPEABLE_PATH_RE));
			assert(!fim::string("file ").re_match(FIM_CNS_PIPEABLE_PATH_RE));
			assert( fim::string("file-").re_match(FIM_CNS_PIPEABLE_PATH_RE));
			if ( fim_common_test() != 0 )
				errval = FIM_ERR_GENERIC;
			{
				fim_char_t s[] = {'\\','x','4','1','\\','a','a','\\','x','4','2','b','\\','b',0};
				trec(s,"ab","cd");
				if ( strcmp(s, "AcaBbd") )
					errval = FIM_ERR_GENERIC;
			}
			for (int si = 0; si < 3; ++si)
			{
				fim_stream stream(si);
				const fim_str_t t = 1;
				const fim_char_t*s = "s";
				const fim_byte_t b = 'b';
				const fim_byte_t S [] = { 'b', FIM_SYM_CHAR_NUL };
				const fim_char_t c = 'c';
				stream << s    << " ";
				stream << b    << " ";
				stream << S    << " ";
				stream << t    << " ";
				stream << 1    << " ";
				stream << 1U   << " ";
				stream << c    << " ";
				stream << 99.f << " ";
				stream << "\n";
			}
#if FIM_WANT_BENCHMARKS
			fim_bench_subsystem(displaydevice_);
			fim_bench_subsystem(&browser_);
			fim_bench_subsystem(this);
			{
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
				const int tbs = 2 * mc_.byte_size();
				std::vector<char> hs(tbs,'?');
				hs[tbs-1] = FIM_SYM_CHAR_NUL;
				CommandConsole::status_screen(hs.data()); // TODO: this is to check if DebugConsole coped with that huge string, so a status check would be in order here.
				displaydevice_->fb_status_screen_new("clear",false,0x01); // TODO: need to connect these to a command
				displaydevice_->fb_status_screen_new("scroll_down",false,0x02);
				displaydevice_->fb_status_screen_new("scroll_up",false,0x03);
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
			}
#endif /* FIM_WANT_BENCHMARKS */
			if ( errval == FIM_ERR_GENERIC )
				return_code_ = -1;
			quit(return_code_);
			exit(return_code_);
		}
		return FIM_ERR_NO_ERROR;
	}

#if FIM_WANT_BENCHMARKS
	fim_int CommandConsole::get_n_qbenchmarks(void)const
	{
		return 1;
	}

	string CommandConsole::get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const
	{
		std::ostringstream oss;
		switch(qbi)
		{
			case 0:
				oss << "fim console random variables set/get test: " << ((float)(((fim_fms_t)qbtimes)/((qbttime)*1.e-3))) << " set/get /s\n";
		}
		return oss.str();
	}

	void CommandConsole::quickbench_init(fim_int qbi)
	{
		std::ostringstream oss;
		switch(qbi)
		{
			case 0:
				oss << "fim console check";
				std::cout << oss.str() << ": " << "please be patient\n";
			break;
		}
	}

	void CommandConsole::quickbench_finalize(fim_int qbi)
	{
	}

	void CommandConsole::quickbench(fim_int qbi)
	{
		switch(qbi)
		{
			case 0:
				FIM_CONSTEXPR fim_int max_sq=1024*1024;
				cc.setVariable(fim_rand()%(max_sq),fim_rand());
				cc.getIntVariable(fim_rand()%max_sq);
			break;
		}
	}
#endif /* FIM_WANT_BENCHMARKS */

	void CommandConsole::dumpDefaultFimrc(void)const
	{
#ifdef FIM_DEFAULT_CONFIGURATION
		std::cout << FIM_DEFAULT_CONFIG_FILE_CONTENTS << "\n";
#endif /* FIM_DEFAULT_CONFIGURATION */
	}
}


