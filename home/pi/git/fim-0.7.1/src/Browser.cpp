/* $LastChangedDate: 2024-05-11 20:15:30 +0200 (Sat, 11 May 2024) $ */
/*
 Browser.cpp : Fim image browser

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
#include <dirent.h>
#include <sys/types.h>	/* POSIX Standard: 2.6 Primitive System Data Types (e.g.: ssize_t) */
#include "fim.h"
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif /* HAVE_LIBGEN_H */

#define FIM_PAN_GOES_NEXT 1
#define FIM_READ_BLK_DEVICES 1
#define FIM_MIN_FILESIZE_FOR_CHANGED 1 /* perhaps it's worth introducing a variable for this */


#define FIM_BROWSER_INSPECT 0
#if FIM_BROWSER_INSPECT
#define FIM_PR(X) printf("BROWSER:%c:%20s: f:%d/%d p:%d/%d %s\n",X,__func__,(int)getGlobalIntVariable(FIM_VID_FILEINDEX),(int)getGlobalIntVariable(FIM_VID_FILELISTLEN),(int)getGlobalIntVariable(FIM_VID_PAGE),/*(getImage()?getImage()->getIntVariable(FIM_VID_PAGES):-1)*/-1,current().c_str());
#else /* FIM_BROWSER_INSPECT */
#define FIM_PR(X) 
#endif /* FIM_BROWSER_INSPECT */

#define FIM_DIFFERENT_VARS FIM_WANT_PIC_LVDN && FIM_USE_CXX11 /* experimental */

#define FIM_GOTO_ONE_JUMP_REGEX "[-+]?[0-9]+[%]?[fp]?"
#define FIM_GOTO_MULTI_JUMP_REGEX "^(" FIM_GOTO_ONE_JUMP_REGEX ")+$"

#include <utility>	/* std::swap */
#include <random>	/* std::random_device */
#include <algorithm>	/* std::shuffle */

#if FIM_WANT_FLIST_STAT 
#include <sstream>	// std::istringstream 
#endif /* FIM_WANT_FLIST_STAT */

#if 0
#if HAVE_WORDEXP_H
#include <wordexp.h>
#else /* HAVE_WORDEXP_H */
#endif /* HAVE_WORDEXP_H */

#if HAVE_GLOB_H
#include <glob.h>
#else /* HAVE_GLOB_H */
#endif /* HAVE_GLOB_H */

#if HAVE_FNMATCH_H
#include <fnmatch.h>
#else /* HAVE_FNMATCH_H */
#endif /* HAVE_FNMATCH_H */
#endif /* 0 */

#if HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */
#if HAVE_TIME_H && ( _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED )
#define FIM_USE_GETDATE
#endif

#ifdef FIM_USE_GETDATE
/* TODO: shall use it like:
	struct tm*tmp;
	tmp=getdate("2009/12/28");
	if(tmp)
		min_mtime_str=*tmp;
*/
#endif /* FIM_USE_GETDATE */

#define FIM_CNS_ENOUGH_FILES_TO_WARN 1000
#define FIM_WITHIN(MIN,VAL,MAX) ((MIN)<=(VAL) && (VAL)<=(MAX))
#define FIM_CNS_Ki 1000
#define FIM_CNS_MANY_DUMPED_TOKENS 1000000
#define FIM_CNS_A_FEW_DUMPED_TOKENS 20
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 2
#include <filesystem>	// c++17 on (201703L)
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */

namespace fim
{
	int Browser::current_n(void)const
	{
	       	return flist_.cf();
	}

	fim_int Browser::current_p(void)const
	{
		// TODO: needs proper diffusion in the code...
		return getGlobalIntVariable(FIM_VID_PAGE);
	}

#if FIM_WANT_PIC_LBFL
	fim_cxr Browser::limit_to_variables(size_t min_vals, bool expanded) const
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		fim_var_id_set ids;

		for(const auto & ns : cc.id_.vd_)
		for(const auto & ip : ns.second)
			if(ip.first.c_str()[0]!='_')
				ids.insert(ip.first); // list unique vars
		std::map<fim_var_id,std::pair<size_t,size_t>> vp; // id -> (vals count, files count)
		std::map<fim_var_id,std::map<std::string,size_t>> vv; // id -> values -> count

		for(const auto & id : ids)
		{
			vp[id].second=0;
			for(const auto & ns : cc.id_.vd_)
			if(ns.second.isSetVar(id))
			{
				vp[id].second++;
				const Var val = ns.second.getVariable(id);
				vv[id][val.getString()]++;
			}
			vp[id].first=vv[id].size();
			if(vp[id].first>=min_vals)
			{
				if (expanded)
				{
					for(const auto & val : vv[id])
						if (val.first.find("  ") == val.first.npos) // would break GTKDevice menu spec otherwise
						{
							const size_t cnt = val.second;
							const auto eid = id + " (" + fim::string(vp[id].first) + ")";
							// menu location is relative ; e.g. "_List/_Limit list/"
							result += eid,
							result += "/",
							result += val.first,
							result += " (" + fim::string(cnt) + ")",
							result += "  ",
							result += "limit ",
							result += fim_shell_arg_escape(id,true),
							result += " ",
							result += fim_shell_arg_escape(val.first,true),
							result += "\n";
						}
				}
				else
				{
					result += id;
					result += ": ";
			       		result += std::to_string(vp[id].first);
					result += " vals / ";
			       		result += std::to_string(vp[id].second);
					result += " files\n";
					for(const auto & val : vv[id])
						result += " ",
						result += val.first,
						result += "\n";
				}
			}
		}
		return result;
	}
#endif /* FIM_WANT_PIC_LBFL */

	fim_cxr Browser::fcmd_list(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;

		FIM_PR('*');
		if(args.size()<1)
		{
			/* returns a string with the info about the files in list (const op) */
			fim::string fileslist;

			for(size_t i=0;i<flist_.size();++i)
				fileslist += fim::string(flist_[i]) + fim::string(" ");
			result = fileslist;
			goto ret;
		}
		else
		{
			/* read-only commands, that do not depend on FIM_VID_LOADING_IN_BACKGROUND */
			if(args[0]=="filesnum")
			{
				result = n_files();
				goto ret;
			}
#if FIM_WANT_FILENAME_MARK_AND_DUMP
			else if(args[0]=="mark" || args[0]=="unmark")
			{
				const bool domark = (args[0]=="mark");
#if FIM_WANT_PIC_LBFL
				if(args.size() > 1)
					result = do_filter_cmd(args_t(args.begin()+1,args.end()),true,domark ? Mark : Unmark);
				else
#endif /* FIM_WANT_PIC_LBFL */
			       		cc.markCurrentFile(domark); 
				goto ret;
		       	} 
			else if(args[0]=="marked")
			{
                                const std::string mfl = cc.marked_files_list();
		                if( mfl != FIM_CNS_EMPTY_STRING )
                                {
                                        result += "The following files have been marked by the user:\n";
                                        result += mfl;
                                }
                                else
                                        result += "No files have been marked by the user.\n";
				goto ret;
		       	} 
			else if(args[0]=="dumpmarked")
			{
				std::cout << cc.marked_files_list();
				goto ret;
			}
			else if(args[0]=="markall")
			{
				int marked = 0;
				for(size_t fi=0;fi<flist_.size();++fi)
					marked += cc.markFile(flist_[fi],true,false);
				result += "marked ";
				result += fim::string(marked);
			       	result +=" files\n";
				goto ret;
			}
			else if(args[0]=="unmarkall")
			{
				cc.marked_files_clear();
				goto ret;
			}
#else /* FIM_WANT_FILENAME_MARK_AND_DUMP */
			else if(args[0]=="mark")
				result = FIM_EMSG_NOMARKUNMARK;
			else if(args[0]=="marked")
				result = FIM_EMSG_NOMARKUNMARK;
			else if(args[0]=="unmark")
				result = FIM_EMSG_NOMARKUNMARK;
			else if(args[0]=="dumpmarked")
				result = FIM_EMSG_NOMARKUNMARK;
			else if(args[0]=="markall")
				result = FIM_EMSG_NOMARKUNMARK;
			else if(args[0]=="unmarkall")
				result = FIM_EMSG_NOMARKUNMARK;
#endif /* FIM_WANT_FILENAME_MARK_AND_DUMP */
#if FIM_WANT_BACKGROUND_LOAD
			else
			if(cc.getIntVariable(FIM_VID_LOADING_IN_BACKGROUND)!=0)
			{
				result = "File list temporarily locked; check out the " FIM_VID_LOADING_IN_BACKGROUND " variable later.";
				goto ret;
			}
#endif /* FIM_WANT_BACKGROUND_LOAD */
			/* commands, that do depend on FIM_VID_LOADING_IN_BACKGROUND */
			if(args[0]=="clear")
				result = _clear_list();
			else if(args[0]=="random_shuffle")
				result = _random_shuffle();
			else if(args[0]=="sort")
				result = _sort(FIM_SYM_SORT_FN);
			else if(args[0]=="sort_basename")
				result = _sort(FIM_SYM_SORT_BN);
			else if(args[0]=="sort_comment")
				result = _sort(FIM_SYM_SORT_BV,FIM_VID_COMMENT);
			else if(args[0]=="sort_var" && args.size()>=2)
				result = _sort(FIM_SYM_SORT_BV,args[1].c_str());
#if FIM_WANT_SORT_BY_STAT_INFO
			else if(args[0]=="sort_mtime")
				result = _sort(FIM_SYM_SORT_MD);
			else if(args[0]=="sort_fsize")
				result = _sort(FIM_SYM_SORT_SZ);
#endif /* FIM_WANT_SORT_BY_STAT_INFO */
			else if(args[0]=="reverse")
				result = _reverse();
			else if(args[0]=="swap")
				result = _swap();
			else if(args[0]=="pop")
				pop(),
				result = this->n_files();
			else if(args[0]=="remove" && args.size()==1)
				pop(FIM_CNS_EMPTY_STRING,true),
				result = this->n_files();
			else if(args[0]=="remove" && args.size()>=2)
				result = do_filter(args_t(args.begin()+1,args.end())/*,FullFileNameMatch,false,Delete*/);
				//result = flist_.pop(args.begin()[1]), // example for alternative
				//result = this->n_files();
			else if(args[0]=="push")
				result = do_push(args_t(args.begin()+1,args.end()));
#ifdef FIM_READ_DIRS
			else if(args[0]=="pushdir")
			{
				if(args.size()>=2)
					push_dir(args[1],FIM_FLAG_NONREC);
				else
					push_dir(".",FIM_FLAG_NONREC);
				result = FIM_CNS_EMPTY_RESULT;
			}
			else if(args[0]=="pushdirr")
			{
#ifdef FIM_RECURSIVE_DIRS
				if(args.size()>=2)
					push_dir(args[1],FIM_FLAG_PUSH_REC);
				else
					push_dir(".",FIM_FLAG_PUSH_REC);
				result = FIM_CNS_EMPTY_RESULT;
#else /* FIM_RECURSIVE_DIRS */
				result = "Please recompile with +FIM_RECURSIVE_DIRS to activate pushdirr.";
#endif /* FIM_RECURSIVE_DIRS */
			}
#endif /* FIM_READ_DIRS */
#if FIM_DIFFERENT_VARS
			else if(args[0]=="variables" || args[0]=="vars")
				result = limit_to_variables(args.size()>1 ? FIM_MAX(std::atoi(args[1]),1) : 2, false);
#endif /* FIM_DIFFERENT_VARS */
			else
				result = FIM_CMD_HELP_LIST;
		}
ret:
		FIM_PR('.');
		return result;
	}

	std::ostream& Browser::print(std::ostream& os)const
	{
		for(auto le : flist_)
			os << le << FIM_CNS_NEWLINE;
		return os;
	}

#ifdef FIM_READ_STDIN_IMAGE
	void Browser::set_default_image(ImagePtr stdin_image)
	{
		FIM_PR('*');
		if( !stdin_image || !stdin_image->check_valid() )
			goto ret;
		default_image_ = std::move(stdin_image);
		if(!cache_.setAndCacheStdinCachedImage(default_image_))
			std::cerr << FIM_EMSG_CACHING_STDIN;
ret:
		FIM_PR('.');
		return;
	}
#endif /* FIM_READ_STDIN_IMAGE */

	Browser::Browser(CommandConsole& cc_):
#ifdef FIM_NAMESPACES
		Namespace(&cc_,FIM_SYM_NAMESPACE_BROWSER_CHAR),
#endif /* FIM_NAMESPACES */
#if FIM_WANT_PIC_LBFL
		flist_(),
#endif /* FIM_WANT_PIC_LBFL */
#if FIM_WANT_PIC_LBFL
		tlist_(),
#endif /* FIM_WANT_PIC_LBFL */
		nofile_(FIM_CNS_EMPTY_STRING),commandConsole_(cc_)
#if FIM_WANT_BACKGROUND_LOAD
		,pcache_(cache_)
#endif /* FIM_WANT_BACKGROUND_LOAD */
	{	
	}

	const fim::string Browser::pop_current(void)
	{
		flist_.pop_current();
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
		return nofile_;
	}

	const fim::string Browser::pop(fim::string filename, bool advance)
	{	
		fim::string s = nofile_;

		if( flist_.size() <= 1 )
			goto ret;

		s = flist_.pop(filename,advance);

		setGlobalVariable(FIM_VID_FILEINDEX,current_image());
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
ret:
		return s;
	}

	fim::string Browser::fcmd_pan(const args_t& args)
	{
		FIM_PR('*');

		if( args.size() < 1 || (!args[0].c_str()) )
			goto nop;

		if( c_getImage() )
		{
			fim_char_t f = tolower(*args[0].c_str());
			if( f )
			{
				FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREPAN,current())
				if( c_getImage() && viewport() )
				{
					const bool pr = viewport()->do_pan(args);
#if FIM_PAN_GOES_NEXT
					fim_char_t s = FIM_SYM_CHAR_NUL;
				       	s = tolower(args[0].end()[-1]);
			       		if( s != FIM_SYM_CHAR_NUL && pr == false )
					{
						/*if( s != '+' && s != '-' )
						switch(f)
						{
							case('u'):
							case('l'):
							case('e'):
							s='-';
							break;
							case('d'):
							case('r'):
							case('w'):
							s='+';
							break;
						}*/
						if( s == '+' )
				       			next(1);
						if( s == '-' )
		       					prev(1);
					}
#endif /* FIM_PAN_GOES_NEXT */
				}
				FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTPAN)
			}
		}
		else
			prev();
nop:
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

#if FIM_EXPERIMENTAL_SHADOW_DIRS
	fim_cxr Browser::shadow_file_swap(const fim_fn_t fn)
	{
		const fim::fle_t tmp = flist_[flist_.cf()];
		flist_[flist_.cf()] = fn;
		loadCurrentImage();
		flist_[flist_.cf()] = tmp;
		viewport()->update_meta(true);
		return "Current image substituted with " + fn;
	}
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */

	fim_cxr Browser::fcmd_scale(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		FIM_PR('*');
		if(viewport())
		{
#if FIM_EXPERIMENTAL_SHADOW_DIRS
			if(args.size() == 1 && args[0] == "shadow")
			{
				const auto bn = std::string(fim_basename_of(current()));
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 1
				fim::fle_t fn;
				for (const auto & e : hlist_)
					if ( bn == std::string(fim_basename_of(e)) )
					{
						fn = e;
						break;
					}
				if ( fn.size() && hlist_.size() > 0 )
					return shadow_file_swap(fn);
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 2
				namespace fs = std::filesystem;
				for ( const auto & shadow_dir: shadow_dirs_ )
				if ( fs::is_directory(shadow_dir) )
				for( const auto & p: fs::recursive_directory_iterator(shadow_dir) )
				{
					if(fs::is_regular_file(p))
					{
						const fim_fn_t cfn = p.path().string().c_str(); // candidate full name
						const fim_fn_t cbn = fim_basename_of(cfn); // candidate base name
						if (cbn == bn)
							return shadow_file_swap(cfn);
					}
				}
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
			}
			else
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
				result = viewport()->img_scale(args,current());
		}
		FIM_PR('.');
		return result;
	}
	
	fim_cxr Browser::fcmd_crop(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
#if FIM_WANT_CROP
		FIM_PR('*');
		if(viewport())
			result = viewport()->img_crop(args,current());
		FIM_PR('.');
#endif /* FIM_WANT_CROP */
		return result;
	}

	fim_cxr Browser::fcmd_color(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		FIM_PR('*');
		if(viewport())
			result = viewport()->img_color(args);
		FIM_PR('.');
		return result;
	}

	fim::string Browser::do_filter_cmd(const args_t args, bool negative, enum FilterAction faction)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;

		/* Interprets a mark/limiting pattern. */
		if(args.size()>0)
		{
#if FIM_WANT_LIMIT_DUPBN
			if( args[0] == "~=" )
			{
				// result = result + "Limiting to duplicate files\n";
				result = do_filter(args,DupFileNameMatch,negative,faction);
			}
			else
			if( args[0] == "~^" )
			{
				// result = result + "Limiting to first occurrences of files\n";
				result = do_filter(args,FirstFileNameMatch,negative,faction);
			}
			else
			if( args[0] == "~$" )
			{
				// result = result + "Limiting to last occurrences of files\n";
				result = do_filter(args,LastFileNameMatch,negative,faction);
			}
			else
			if( args[0] == "~!" )
			{
				// result = result + "Limiting to unique files\n";
				result = do_filter(args,UniqFileNameMatch,negative,faction);
			}
			else
			if( args[0] == "~i" && args.size()>1 )
			{
				// result = result + "Limiting according to list interval\n";
				result = do_filter(args,ListIdxMatch,negative,faction);
			}
			else
#if FIM_WANT_FLIST_STAT 
			if( args[0] == "~d" )
			{
				// result = result + "Limiting according to time interval\n";
				result = do_filter(args,TimeMatch,negative,faction);
			}
			else
			if( args[0] == "~z" )
			{
				// result = result + "Limiting according to file size\n";
				result = do_filter(args,SizeMatch,negative,faction);
			}
			else
#else /* FIM_WANT_FLIST_STAT */
			if( args[0] == "~d" )
			{
				result = result + "~d unsupported\n";
			}
			else
			if( args[0] == "~z" )
			{
				result = result + "~z unsupported\n";
			}
#endif /* FIM_WANT_FLIST_STAT */
#else  /* FIM_WANT_LIMIT_DUPBN */
				result = result + "Limited options unsupported.\n";
#endif /* FIM_WANT_LIMIT_DUPBN */
#if FIM_WANT_FILENAME_MARK_AND_DUMP
			if( args[0] == "!" )
			{
				// result = result + "Limiting to marked files\n";
				result = do_filter(args,MarkedMatch,!negative,faction); /* this is remove on filenames; need remove on comments */
			}
			else
#endif /* FIM_WANT_FILENAME_MARK_AND_DUMP */
				result = do_filter(args,VarMatch,!negative,faction); /* this is remove on filenames; need remove on comments */
		}
		return result;
	}

#if FIM_WANT_PIC_LBFL
	fim_cxr Browser::fcmd_limit(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		const int aoc = fim_args_opt_count(args,'-');

		if( args.size()-aoc > 0 )
		{
			const fim_int lbl = n_files(); // length before limiting
			const enum FilterAction faction = Delete;
			flist_t clist; // current list copy

#if FIM_WANT_PIC_LVDN
			if( args.size()-aoc == 1 && aoc == 1 && 
				( fim_args_opt_have(args,"-list") || fim_args_opt_have(args,"-listall" ) ) )
			{
				size_t max_vars = FIM_CNS_A_FEW_DUMPED_TOKENS;
				if( fim_args_opt_have(args,"-listall"))
					max_vars=FIM_CNS_MANY_DUMPED_TOKENS;
				result = cc.id_.get_values_list_for(args[1],max_vars) + string("\n");
				goto nop;
			}
#endif /* FIM_WANT_PIC_LVDN */

			if(!limited_)
				limited_ = true,
				tlist_ = flist_; // a copy

			if(                  fim_args_opt_have(args,"-merge"))
			       	clist = flist_; // current list
			else
			if(                  fim_args_opt_have(args,"-subtract"))
			       	clist = flist_; // current list

			if(tlist_.size() && !fim_args_opt_have(args,"-further"))
			       	flist_ = tlist_; // limit on total list

			result = do_filter_cmd(args_t(args.begin()+aoc,args.end()),false,faction);

			if(clist.size())
			{
				clist._sort(FIM_SYM_SORT_FN);
				if(fim_args_opt_have(args,"-merge"))
					flist_._set_union(clist);
				else
				if(fim_args_opt_have(args,"-subtract"))
					flist_._set_difference_from(clist);
			}

			if(n_files() != lbl)
				reload();
		}
		else
		{
#if FIM_WANT_PIC_LVDN
			if( args.size()-aoc == 0 && aoc == 1 && 
				( fim_args_opt_have(args,"-list") || fim_args_opt_have(args,"-listall" ) ) )
			{
				size_t max_vals = FIM_CNS_A_FEW_DUMPED_TOKENS;
				if( fim_args_opt_have(args,"-listall"))
					max_vals=FIM_CNS_MANY_DUMPED_TOKENS;
				result = cc.id_.get_variables_id_list(max_vals) + string("\n");
				goto nop;
			}
#endif /* FIM_WANT_PIC_LVDN */

			if(limited_)
			{
				// result = "Restoring the original browsable files list.";
			       	commandConsole_.set_status_bar("restoring files list", "*");
				flist_ = tlist_; // original list
				limited_ = false;
			}
		}
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
nop:
		return result;
	}
#endif /* FIM_WANT_PIC_LBFL */

	fim::string Browser::display_status(const fim_char_t *l)
	{
		FIM_PR('*');

		if( getGlobalIntVariable(FIM_VID_DISPLAY_STATUS) == 1 )
		{
			fim::string dss;
			const fim_char_t *dssp=FIM_NULL;

			if( cc.isSetVar(FIM_VID_DISPLAY_STATUS_FMT) )
				dss = cc.getInfoCustom(cc.getStringVariable(FIM_VID_DISPLAY_STATUS_FMT).c_str());
			dssp=dss.c_str();
			commandConsole_.set_status_bar( (dssp && *dssp ) ? dssp : l, c_getImage()?(c_getImage()->getInfo().c_str()):"*");
		}
		else
		{
			if( fim_bool_t wcs = cc.isSetVar(FIM_VID_WANT_CAPTION_STATUS) )
				wcs = cc.set_wm_caption(FIM_NULL);
		}
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

#if FIM_WANT_FAT_BROWSER
#if 0
	fim_cxr Browser::fcmd_no_image(const args_t& args)
	{
		/* sets no image as the current one */
		FIM_PR('*');
		free_current_image();
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}
#endif
#endif /* FIM_WANT_FAT_BROWSER */

	int Browser::load_error_handle(fim::string c)
	{
		/*
		 * assume there was a load attempt: check and take some action in case of error
		 *
		 * FIXME: this behaviour is BUGGY, because recursion will be killed off 
		 *         by the autocommand loop prevention mechanism. (this is not true, as 20090215)
		 * */
		static int lehsof = 0;	/* './fim FILE NONFILE' and hitting 'prev' will make this necessary  */
		int retval = 0;
		FIM_PR('*');

		if( lehsof )
			goto ret; /* this prevents infinite recursion */
		if( /*c_getImage() &&*/ viewport() && ! (viewport()->check_valid()) )
		{
    	    		if ( cc.getIntVariable(FIM_VID_VERBOSITY) )
	    			std::cout << "loading " << c << " has failed" << "\n"; 
			free_current_image(false);
			++ lehsof;
#ifdef FIM_REMOVE_FAILED
				//pop(c);	//removes the currently specified file from the list. (pop doesn't work in this way)
				do_filter({c});
#if FIM_WANT_GOTOLAST
				/* on failed load, try jumping to last meaningful file */
				{
					const fim_int fi = getGlobalIntVariable(FIM_VID_FILEINDEX);
					const fim_int li = getGlobalIntVariable(FIM_VID_LASTFILEINDEX);
					if ( fi > li && li > 0 )
						setGlobalVariable(FIM_VID_FILEINDEX, li),
						flist_.set_cf(li-1);
				}
#endif
#ifdef FIM_AUTOSKIP_FAILED
				if(n_files())
				{
					//next(1);
					reload(); /* this is effective, at least partially */
				}
#endif /* FIM_AUTOSKIP_FAILED */
#endif /* FIM_REMOVE_FAILED */
			--lehsof;
			retval = 1;
		}
ret:
		FIM_PR('.');
		return retval;
	}

	fim::string Browser::reload(void)
	{
		if( n_files() )
			return fcmd_reload({});
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_err_t Browser::loadCurrentImage(void)
	{
		// this function needs revision
		const fim_err_t errval = FIM_ERR_NO_ERROR;

		FIM_PR('*');
		try
		{
	#ifdef FIM_CACHE_DEBUG
		if( viewport() ) std::cout << "browser::loadCurrentImage(\"" << current().c_str() << "\")\n";
	#endif /* FIM_CACHE_DEBUG */
		if( viewport()
			&& !( current()!=FIM_STDIN_IMAGE_NAME && !is_file(current()) ) /* FIXME: this is an unelegant fix to prevent crashes on non-existent files. One shall better fix this by a good exception mechanism for Image::Image() and a clean approach w.r.t. e.g. free_current_image(true) */
		)
		{
			ViewportState viewportState;
			FIM_PR('0');
			viewport()->setImage(
#if FIM_WANT_BACKGROUND_LOAD
					pcache_.
#else /* FIM_WANT_BACKGROUND_LOAD */
					 cache_.
#endif /* FIM_WANT_BACKGROUND_LOAD */
					useCachedImage(
						cache_key_t{current(),
						  {getGlobalIntVariable(FIM_VID_PAGE), ((current()==FIM_STDIN_IMAGE_NAME)?FIM_E_STDIN:FIM_E_FILE)}}, &viewportState));// FIXME
				viewport()->ViewportState::operator=(viewportState);
				setGlobalVariable(FIM_VID_FILEINDEX,current_image());
		}
		}
		catch(FimException e)
		{
			FIM_PR('E');
			if(viewport())
				viewport()->setImage( FIM_NULL );
		}
		FIM_PR('.');
		return errval;
	}

	void Browser::free_current_image(bool force)
	{
		FIM_PR('*');
		if( viewport() )
			viewport()->free_image(force);
		setGlobalVariable(FIM_VID_CACHE_STATUS,cache_.getReport());
		FIM_PR('.');
	}

	fim_cxr Browser::fcmd_prefetch(const args_t& args)
	{
		fim_int wp=1;
		FIM_PR('*');

		FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREPREFETCH,current())
		if( args.size() > 0 )
			goto ret;

		wp = getGlobalIntVariable(FIM_VID_WANT_PREFETCH);
		setGlobalVariable(FIM_VID_WANT_PREFETCH,0);
#if FIM_WANT_BACKGROUND_LOAD
		if(wp == 2)
		{
			pcache_.asyncPrefetch(get_next_filename(1));
			pcache_.asyncPrefetch(get_next_filename(-1));
			goto apf;
		}
#endif /* FIM_WANT_BACKGROUND_LOAD */
		if(cache_.prefetch(get_next_filename( 1).c_str()))// we prefetch 1 file forward
#ifdef FIM_AUTOSKIP_FAILED
			pop(get_next_filename( 1));/* if the filename doesn't match a loadable image, we remove it */
#else /* FIM_AUTOSKIP_FAILED */
			{}	/* beware that this could be dangerous and trigger loops */
#endif /* FIM_AUTOSKIP_FAILED */
		if(cache_.prefetch(get_next_filename(-1)))// we prefetch 1 file backward
#ifdef FIM_AUTOSKIP_FAILED
			pop(get_next_filename(-1));/* if the filename doesn't match a loadable image, we remove it */
#else /* FIM_AUTOSKIP_FAILED */
			{}	/* beware that this could be dangerous and trigger loops */
#endif /* FIM_AUTOSKIP_FAILED */
#if FIM_WANT_BACKGROUND_LOAD
apf:		/* after prefetch */
#endif /* FIM_WANT_BACKGROUND_LOAD */
		FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTPREFETCH)
		setGlobalVariable(FIM_VID_WANT_PREFETCH,wp);
		if(wp != 2)
			display_status("");
ret:
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_cxr Browser::fcmd_reload(const args_t& args)
	{
		fim::string result;

		FIM_PR('*');
		//for(size_t i=0;i<args.size();++i) push_path(args[i]);
		if( empty_file_list() )
		{
		       	result = "sorry, no image to reload\n"; 
			goto ret;
	       	}
		FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PRERELOAD,current())
		if( args.size() > 0 )
			free_current_image(true);
		else
			free_current_image(false);
		loadCurrentImage();

//		while( n_files() && viewport() && ! (viewport()->check_valid() ) && load_error_handle(c) );
		load_error_handle(_c);
		FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTRELOAD)
		result = FIM_CNS_EMPTY_RESULT;
ret:
		FIM_PR('.');
		return result;
	}

	fim_cxr Browser::fcmd_load(const args_t& args)
	{
		/*
		 * loads the current file, if not already loaded
		 */
		fim::string result = FIM_CNS_EMPTY_RESULT;
		FIM_PR('*');

		//for(size_t i=0;i<args.size();++i) push_path(args[i]);
		if( c_getImage() && ( c_getImage()->getName() == current()) )
		{
			result = "image already loaded\n";//warning
			goto ret;
		}
		if( empty_file_list() )
		{
			result = "sorry, no image to load\n";//warning
			goto ret;
		}
		FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PRELOAD,current())
		commandConsole_.set_status_bar("please wait while loading...", "*");

		loadCurrentImage();

		load_error_handle(_c);
		FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTLOAD)
ret:
		FIM_PR('.');
		return result;
	}

	fim_int Browser::find_file_index(const fim::string nf)const
	{
		fim_int fi = -1;
		for(flist_t::const_iterator fit=flist_.begin();fit!=flist_.end();++fit)
			if( string(*fit) == nf )
			{
				fi = fit - flist_.begin();
				goto ret;
			}
ret:
		return fi;
	}

	bool Browser::present(const fim::string nf)const
	{
		bool ip = false;

		if( find_file_index(nf) >= 0 )
			ip = true;
		return ip;
	}

#ifdef FIM_READ_DIRS
	bool Browser::push_dir(const fim::string & nf, fim_flags_t pf, const fim_int * show_must_go_on)
	{
		DIR *dir = FIM_NULL;
		struct dirent *de = FIM_NULL;
		fim::string dn=nf;
		bool retval = false;
		FIM_PR('*');
		fim::string re;

		if( show_must_go_on && !*show_must_go_on )
			goto rret;

		if(cc.getIntVariable(FIM_VID_PRELOAD_CHECKS)!=1)
			goto nostat;
		/*	we want a dir .. */
#ifdef HAVE_LIBGEN_H
		if( !is_dir( dn.c_str() ) )
			dn = fim_dirname(dn);
#else /* HAVE_LIBGEN_H */
		if( !is_dir( dn ))
			goto ret;
#endif /* HAVE_LIBGEN_H */

nostat:
#if FIM_WANT_PROGRESS_RECURSIVE_LOADING
		if( pf & FIM_FLAG_PUSH_BACKGROUND )
		{
			static int maxsize=0;
			const char * FIM_CLEARTERM_STRING = "\x1B\x4D"; /* man terminfo */
			std::cout << FIM_CLEARTERM_STRING << "scanning: " << flist_.size()<< dn << std::endl;
			maxsize = FIM_MAX(dn.size(),maxsize);
		}
#endif
		if ( ! ( dir = opendir(dn.c_str() ) ))
			goto ret;
		if ( dn.back() != FIM_CNS_DIRSEP_CHAR )
			dn += FIM_CNS_DIRSEP_STRING;
	       	re = getGlobalStringVariable(FIM_VID_PUSHDIR_RE);
		if( re == FIM_CNS_EMPTY_STRING )
			re = FIM_CNS_PUSHDIR_RE;
		//are we sure -1 is not paranoid ?
		while( ( de = readdir(dir) ) != FIM_NULL )
		{
			if( de->d_name[0] == '.' && !de->d_name[1] )
				continue;
			if( de->d_name[0] == '.' &&  de->d_name[1] == '.' && !de->d_name[2] )
				continue;
#if FIM_RECURSIVE_HIDDEN_DIRS_SKIP_CHECK
			/* We follow the convention of ignoring hidden files.  */
			if( (!(pf & FIM_FLAG_PUSH_HIDDEN)) && de->d_name[0] == '.' )
				continue;
#endif /* FIM_RECURSIVE_HIDDEN_DIRS_SKIP_CHECK */
		{
			/* Note: Following circular links may cause memory exhaustion.  */
			const fim_fn_t pn = dn + fim_fn_t(de->d_name);

			if( is_dir( pn ) )
			{
#ifdef FIM_RECURSIVE_DIRS
				if( pf & FIM_FLAG_PUSH_REC )
					retval |= push_dir( pn, pf, show_must_go_on);
				else
#endif /* FIM_RECURSIVE_DIRS */
					continue;
			}
			else 
			{
				if( pn.re_match(re.c_str()) )
					retval |= push_path( pn, pf | FIM_FLAG_PUSH_FILE_NO_CHECK );
			}
		}
#if FIM_WANT_BACKGROUND_LOAD
			if( ( pf & FIM_FLAG_PUSH_ONE ) && retval )
				goto ret;
#endif /* FIM_WANT_BACKGROUND_LOAD */
		}
ret:
		//retval = ( closedir(dir) == 0 );
		closedir(dir);
rret:
		FIM_PR('.');
		return retval;
	}
#endif /* FIM_READ_DIRS */

	bool Browser::push_path(const fim::string & nf, fim_flags_t pf, const fim_int * show_must_go_on)
	{
		bool pec = false; // push error code
#if 0
		int eec = 0; // expansion error code
#if HAVE_WORDEXP_H
	{
		wordexp_t p;
		eec = wordexp(nf.c_str(),&p,0);
		if( eec == 0 )
		{
			for(size_t g=0;g<p.we_wordc;++g)
				pec |= push_noglob(p.we_wordv[g],pf,show_must_go_on);
			wordfree(&p);
			goto ret;
		}
	}
#endif /* HAVE_WORDEXP_H */

#if HAVE_GLOB_H
	{
               	glob_t pglob;
               	pglob.gl_offs = 0;
       		eec = glob(nf.c_str(),GLOB_NOSORT| GLOB_NOMAGIC | GLOB_TILDE, FIM_NULL, &pglob);
		if( eec == 0 )
		{
			for(size_t g=0;g<pglob.gl_pathc;++g)
				pec |= push_noglob(pglob.gl_pathv[g],pf,show_must_go_on);
			globfree(&pglob);
			goto ret;
		}
	}
#endif /* HAVE_GLOB_H */
#endif /* 0 */
		pec = push_noglob(nf.c_str(),pf,show_must_go_on);
// ret:
		return pec;
	}

	bool Browser::push_noglob(const fim::string & nf, fim_flags_t pf, const fim_int * show_must_go_on)
	{	
		bool retval = false;
		bool lib = false; /* load in background */

#if FIM_WANT_BACKGROUND_LOAD
		if( pf & FIM_FLAG_PUSH_BACKGROUND )
			lib = true;
#endif /* FIM_WANT_BACKGROUND_LOAD */
		FIM_PR('*');

		if( pf & FIM_FLAG_PUSH_FILE_NO_CHECK )
			goto isfile;

		if( nf == FIM_STDIN_IMAGE_NAME )
			goto isfile;

		if(cc.getIntVariable(FIM_VID_PRELOAD_CHECKS)!=1)
		{
			const size_t sl = strlen(nf.c_str());

			if(sl < 1)
				goto ret;

			if( nf[sl-1] == FIM_CNS_SLASH_CHAR )
				goto isdir;
			else
				goto isfile;
		}

		{
#ifdef FIM_CHECK_FILE_EXISTENCE 
#ifdef HAVE_SYS_STAT_H 
			/*
			 * skip adding filename to list if not existent or is a directory...
			 */
			struct stat stat_s;

			/*	if the file doesn't exist, return */
			if( -1 == stat(nf.c_str(),&stat_s) )
			{
#if 0
				if( errno != EOVERFLOW) /* this may happen with a readable file...  */
#endif
				{
					/* fim_perror("!"); */
					goto ret;
				}
			}
			/*	if it is a character device , return */
			//if(  S_ISCHR(stat_s.st_mode))return FIM_CNS_EMPTY_RESULT;
			/*	if it is a block device , return */
			//if(  S_ISBLK(stat_s.st_mode))return FIM_CNS_EMPTY_RESULT;
			/*	if it is a directory , return */
			//if(  S_ISDIR(stat_s.st_mode))return FIM_CNS_EMPTY_RESULT;
#ifdef FIM_READ_DIRS
//#if HAVE_SYS_STAT_H
			{
				const fim_int ppd = getGlobalIntVariable(FIM_VID_PUSH_PUSHES_DIRS);
				if( ppd >= 1 && S_ISDIR(stat_s.st_mode))
				{
#if FIM_RECURSIVE_HIDDEN_DIRS_SKIP_CHECK
					if( ppd > 1 )
						pf |= FIM_FLAG_PUSH_HIDDEN;
#endif /* FIM_RECURSIVE_HIDDEN_DIRS_SKIP_CHECK */
					goto isdir;
				}
			}
//#endif /* HAVE_SYS_STAT_H */
#endif /* FIM_READ_DIRS */
			/*	we want a regular file .. */
			if(
				! S_ISREG(stat_s.st_mode) 
#ifdef FIM_READ_BLK_DEVICES
				&& ! S_ISBLK(stat_s.st_mode)  // NEW
#endif /* FIM_READ_BLK_DEVICES */
			)
			{
				if(!lib)
					commandConsole_.set_status_bar((nf+" is not a regular file!").c_str(), "*");
				goto ret;
			}
#endif /* HAVE_SYS_STAT_H */
#endif /* FIM_CHECK_FILE_EXISTENCE */
		}
isfile:
#ifdef FIM_CHECK_DUPLICATES
		if( ( ! ( pf & FIM_FLAG_PUSH_ALLOW_DUPS ) ) && present(nf) )
		{
			//there could be an option to have duplicates...
			//std::cout << "no duplicates allowed..\n";
			goto ret;
		}
#endif /* FIM_CHECK_DUPLICATES */
		flist_.push_back(nf);
		if(cc.getVariable(FIM_VID_DBG_COMMANDS).find('B') >= 0)
			std::cout << FIM_CNS_CLEARTERM << FIM_CNS_DBG_CMDS_PFX << " pushing " << nf << FIM_CNS_NEWLINE;
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
		retval = true;
		goto ret;
isdir:
#ifdef FIM_READ_DIRS
		retval = push_dir(nf,pf,show_must_go_on);
#endif /* FIM_READ_DIRS */
ret:
		FIM_PR('.');
		return retval;
	}

#if FIM_EXPERIMENTAL_SHADOW_DIRS
	void Browser::push_shadow_dir(std::string fn)
	{
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 1
		// this is slow because of FIM_FLAG_PUSH_REC
		std::swap(flist_,hlist_);
		push_dir(fn,FIM_FLAG_PUSH_REC,NULL);
		std::swap(flist_,hlist_);
#endif
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 2
		// this is fast because we search for the shadow file when needed
		shadow_dirs_.push_back(fn);
#endif
	}
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
	
	fim_int Browser::n_files(void)const
	{
		return flist_.size();
	}

	void Browser::_unique(void)
	{
		// makes only sense if flist_ sorted.
		flist_._unique();
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
	}

	fim::string Browser::_sort(const fim_char_t sc, const char*id)
	{
		flist_._sort(sc,id);
		return current();
	}

	fim::string Browser::_random_shuffle(bool dts)
	{
		/*
		 *	sorts image filenames list
		 *	if dts==true, do time() based seeding
		 *	TODO: it would be cool to support a user supplied seed value
		 */
		std::random_device rd;
		std::mt19937 g(rd());
		if( dts )
			g.seed(time(FIM_NULL));
		else
			g.seed(std::mt19937::default_seed);
		std::shuffle(flist_.begin(),flist_.end(),g);
		return current();
	}

	fim::string Browser::_clear_list(void)
	{
		flist_.erase(flist_.begin(),flist_.end());
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
		return FIM_CNS_EMPTY_RESULT;
	}

	fim::string Browser::_reverse(void)
	{
		std::reverse(flist_.begin(),flist_.end());
		return current();
	}

	fim::string Browser::_swap(void)
	{
		/*
		 *	swap current with first
		 */
		if( n_files() > 1 && current_n() )
			std::swap(flist_[0],flist_[current_n()]);
		return current();
	}

	fim::string Browser::regexp_goto(const args_t& args, fim_int src_dir)
	{
		/*
		 * goes to the next filename-matching file
		 * TODO: this member function shall only find the index and return it !
		 */
		const size_t c = current_n(),s = flist_.size();
		size_t i,j;
		const fim::string rso = cc.isSetVar(FIM_VID_RE_SEARCH_OPTS) ? cc.getStringVariable(FIM_VID_RE_SEARCH_OPTS) : "bi";
		int rsic = 1; /* ignore case */
		int rsbn = 1; /* base name */
		FIM_PR('*');

		if ( rso && strchr(rso,'i') )
			rsic = 1;
		else
			if ( rso && strchr(rso,'I') )
				rsic = 0;
		if ( rso && strchr(rso,'b') )
			rsbn = 1;
		else
			if ( rso && strchr(rso,'f') )
				rsbn = 0;

		if( args.size() < 1 || s < 1 )
			goto nop;

		last_regexp_ = args[0];
		last_src_dir_ = src_dir;

		for(j=0;j<s;++j)
		{
			const fim_char_t *fstm = FIM_NULL;
			bool hm = false;

			i = ((src_dir<0?(s-j):j)+c+src_dir)%s;

			if(!(fstm = flist_[i].c_str()))
				continue;

			if(rsbn==1)
				fstm = fim_basename_of(fstm);

			hm = (commandConsole_.regexp_match(fstm,args[0].c_str(),rsic));
#if FIM_WANT_PIC_CMTS
			/* If filename does not match, we look for match on description. */
			if(!hm)
			if ( rso && strchr(rso,'D') )
			{
				/* FIXME: in the long run need a uniform decision on whether comments are to be accessed by full path or basename. */
				const fim_char_t * bfstm = fim_basename_of(fstm);
				if(cc.id_.find(fim_fn_t(bfstm)) != cc.id_.end() )
					bfstm = (cc.id_[fim_fn_t(bfstm)]).c_str();
				hm = (commandConsole_.regexp_match(bfstm,args[0].c_str(),rsic));
			}
#endif /* FIM_WANT_PIC_CMTS */

			if(hm)
			{	
				FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREGOTO,current())
				goto_image(i);
				FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTGOTO)
#ifdef FIM_AUTOCMDS
				if(!commandConsole_.inConsole())
					commandConsole_.set_status_bar((current()+fim::string(" matches \"")+args[0]+fim::string("\"")).c_str(),FIM_NULL);
				goto nop;
#endif /* FIM_AUTOCMDS */
			}
		}
		cout << "sorry, no filename matches \""<<args[0]<<"\"\n";
		if(!commandConsole_.inConsole())
			commandConsole_.set_status_bar((fim::string("sorry, no filename matches \"")+
						args[0]+
						fim::string("\"")).c_str(),FIM_NULL);
nop:
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

	fim::string Browser::goto_image(fim_int n, bool isfg)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		const auto N = flist_.size();
		FIM_PR('*');

		if( !N )
			goto ret;

		if( !isfg )
#if FIM_WANT_BDI
		if( N==1 &&              c_getImage()->is_multipage())
#else	/* FIM_WANT_BDI */
		if( N==1 && c_getImage() && c_getImage()->is_multipage())
#endif	/* FIM_WANT_BDI */
		{
			//if(1)std::cout<<"goto page "<<n<<FIM_CNS_NEWLINE;
			FIM_PR(' ');
			if(viewport())
				viewport()->img_goto_page(n);
			result = N;
			goto ret;
		}
#if FIM_WANT_GOTOLAST
		// TODO: need page level memory/jump, too (see FIM_VID_LASTPAGEINDEX)
		if(getGlobalIntVariable(FIM_VID_LASTFILEINDEX) != current_image())
			setGlobalVariable(FIM_VID_LASTFILEINDEX, current_image());
		flist_.set_cf(n);
#if FIM_WANT_LASTGOTODIRECTION
		if( ( n_files() + current_image() - getGlobalIntVariable(FIM_VID_LASTFILEINDEX) ) % n_files() > n_files() / 2 )
			setGlobalVariable(FIM_VID_LASTGOTODIRECTION,"-1");
		else
			setGlobalVariable(FIM_VID_LASTGOTODIRECTION,"+1");
#endif /* FIM_WANT_LASTGOTODIRECTION */
#endif /* FIM_WANT_GOTOLAST */
		FIM_PR(' ');
		setGlobalVariable(FIM_VID_PAGE,FIM_CNS_FIRST_PAGE);
		setGlobalVariable(FIM_VID_FILEINDEX,current_image());
		//setGlobalVariable(FIM_VID_FILEINDEX,cf_);
		setGlobalVariable(FIM_VID_FILENAME, current());
		FIM_PR(' ');
		//loadCurrentImage();
		result = n_files()?(flist_[current_n()]):nofile_;
ret:
		FIM_PR('.');
		return result;
	}

	fim::string Browser::get_next_filename(int n)const
	{
		/*
		 * returns to the next image in the list, the mechanism
		 * p.s.: n<>0
		 */
		if( !flist_.size() )
			return FIM_CNS_EMPTY_RESULT;
		return flist_[FIM_MOD(flist_.cf() + n,flist_.size())];
	}

	fim::string Browser::next(int n)
	{
		fim::string gs = "+";

		gs += fim::string(n);
		return goto_image_internal(gs.c_str(),FIM_X_NULL);  
	}

	fim::string Browser::prev(int n)
	{
		fim::string gs = "-";
		
		gs += fim::string(n);
		return goto_image_internal(gs.c_str(),FIM_X_NULL);  
	}
	
	fim_cxr Browser::fcmd_goto(const args_t& args)
	{
		fim::string result = FIM_CNS_EMPTY_RESULT;
		if( args.size() > 0 )
		{
			const fim_int cfi = current_n();
			const fim_int cpi = current_p();
			for(size_t i = 0; i<args.size() && current_n() == cfi && current_p() == cpi ; ++i)
			{
				result = goto_image_internal(args[i].c_str(),FIM_X_NULL);
			}
		}
		else
			result = goto_image_internal(FIM_NULL,FIM_X_NULL);
		return result;
	}

	fim_goto_spec_t Browser::goto_image_compute(const fim_char_t *s,fim_xflags_t xflags)const
	{
		fim_goto_spec_t gss;
		const fim_char_t*errmsg = FIM_CNS_EMPTY_STRING;
		const int cf = flist_.cf(), cp =getGlobalIntVariable(FIM_VID_PAGE),pc = FIM_MAX(1,n_pages());
		const fim_int fc = n_files();
		fim_int gv = 0,nf = cf,mv = 0,np = cp;
		FIM_PR('*');

		if( n_files() == 0 || !s )
		{
			errmsg = FIM_CMD_HELP_GOTO;
			goto err;
		}
		if(!s)
		{
			errmsg = FIM_CMD_HELP_GOTO;
			goto err;
		}
		else
		{
			fim_char_t c = FIM_SYM_CHAR_NUL;
			fim_char_t l = FIM_SYM_CHAR_NUL;
			int sl = 0,li = 0;
			bool pcnt = false;
			bool isre = false;
			bool ispg = false;
			bool isfg = false; // is file goto
			bool isrj = false;
			bool isdj = false;
			bool isac = false; // accelerated
			bool isbl = false; // boundaries limited [0...mv-1]
#if FIM_WANT_NEXT_ACCEL
			extern fim_int same_keypress_repeats;
#endif /* FIM_WANT_NEXT_ACCEL */

			if( !s )
				goto ret;
			sl = strlen(s);
			if( sl < 1 )
				goto ret;
			c = *s;
			//for(li=sl-2;li<sl;++li) { l=tolower(s[li]); pcnt=(l=='%'); ispg=(l=='p'); }
			l = tolower(s[li=sl-1]);
			pcnt = (l=='%'); 
			ispg = (l=='p');
			isfg = (l=='f');
			isre = ((sl>=2) && ('/'==s[sl-1]) && (((sl>=3) && (c=='+' || c=='-') && s[1]=='/') ||( c=='/')));
			isrj = (c=='+' || c=='-');
			isdj = isrj && s[1] == '/' && ( sl == 2 || ( sl == 3 && strchr("udsbUDSB",s[2])) );
#if FIM_WANT_NEXT_ACCEL
			isac = !isre && isrj && isupper(s[sl-1]) && same_keypress_repeats>0;
			isbl = isac;
#endif /* FIM_WANT_NEXT_ACCEL */

#if FIM_WANT_GOTO_DIR
			if( isdj )
			{
				const int neg=(c=='-'?-1:1);
				std::string bcn;
				const char ct = sl < 3 ? 'S' : s[2];

				switch(tolower(ct))
				{
					case 'u':
					case 'd':
					case 's':
						bcn = fim_dirname(current()); break;
					case 'b':
						bcn = fim_basename_of(current()); break;
				}

				for(fim_int fi=0;fi<fc;++fi)
				{
					const size_t idx=FIM_MOD((cf+neg*(1+fi)+fc),fc);
					std::string icn;
					bool match=false;

					switch(tolower(ct))
					{
						case 'u':
						case 'd':
						case 's':
						//case 'l':
						//case 'm':
				       			icn = fim_dirname(flist_[idx]); break;
						case 'b':
							icn = fim_basename_of(flist_[idx]); break;
					}

					switch(tolower(ct))
					{
						//case 'l':
							//match = icn <  bcn; break;
						//case 'm':
							//match = icn >  bcn; break;
						case 'u':
							match = icn.length() <  bcn.length() && bcn.compare(0,icn.length(),icn) == 0 ; break;
						case 'd':
							match = bcn.length() <  icn.length() && icn.compare(0,bcn.length(),bcn) == 0 ; break;
						case 's':
						case 'b':
							match = bcn == icn; break;
					}

					if(isupper(ct))
						match = ! match;

					if(match)
					{
						nf=idx;
						goto go_jump;
					}
				}
			}
#endif /* FIM_WANT_GOTO_DIR */
#if FIM_WANT_VAR_GOTO
			if( isrj && s[1] && ( isalpha(s[1]) || s[1] == '_' ) )
			{
				const char * ib = s+1, *ie = s+2;
				const int neg=(c=='-'?-1:1);
				bool ner=false; // non empty requirement
				std::map<std::string,std::string> vals;
				std::vector<std::string> keys; // ordered

				do
				{
					while(*ie && ( isalpha(*ie) || *ie == '_' || isdigit(*ie) ) )
						++ie;
					std::string varname(ib,ie);
					keys.push_back(varname);
					std::string varval = cc.id_.vd_[fim_basename_of(current())].getStringVariable(varname);
					vals[varname]=varval;
				} while(*ie=='|' && *(ib=++ie));
				if(*ie == '+' )
					ner = true;

				for(fim_uint fi=0;fi<fc;++fi)
				for(fim_uint ii=0;ii<keys.size();++ii)
				{
					const std::string varname(keys[ii]);
					const std::string varval(vals[varname]);
					const size_t idx = FIM_MOD((cf+neg*(1+fi)+fc),fc);
					const std::string nvarval = cc.id_.vd_[fim_basename_of(flist_[idx])].getStringVariable(varname);
					if(nvarval != varval && ! ( ner == true && !nvarval.size() ) )
					{
						//std::cout<<"["<<varname<<"]="<<varval<<" -> "<<nvarval<<std::endl;
						//std::cout<<cf<<" -> "<<idx<<std::endl;
						nf=idx;
						goto go_jump;
					}
				}
			}
#endif /* FIM_WANT_VAR_GOTO */
			if( isdigit(c)  || c == '-' || c == '+' )
			{
				gv = fim_atoi(s);
				if(gv == FIM_CNS_LAST)
					gv = -1;
			}
			else
			       	if( c == '^' || c == 'f' )
					gv = 1;
			else
			       	if( c == '$' || c == 'l' )
					gv = -1;// temporarily
			else
			if( c == '?' )
			{
				gv = find_file_index(string(s).substr(1,sl-1));
				if( gv < 0 )
				{
					goto ret;
				}
				nf = gv;
				goto go_jump;
			}
			else
				if( isre )
					{ }
			else
			{
				cout << " please specify a number or ^ or $\n";
			}
			if( li > 0 && ( isfg || pcnt || ispg ))
			{
				l = tolower( s[ li = sl-2 ] );
				if( l == '%' )
					pcnt = true;
				if( l == 'p' )
					ispg = true;
				if( l == 'f' )
					isfg = true;
			}
			if( c=='$' || c == 'l' )
				gv = mv - 1;
			if( (isrj) && (!isfg) && (!ispg) && pc > 1 )
			{
				ispg = true;
				if(( cp == 0 && gv < 0 ) || (cp == pc-1 && gv > 0 ) )
					if( fc > 1 )
						isfg = true, ispg = false;
			}
#if FIM_WANT_NEXT_ACCEL
			if( isac )
				gv *= std::pow(2,FIM_MIN(same_keypress_repeats,12));
#endif /* FIM_WANT_NEXT_ACCEL */
			if( ispg )
				mv = pc;
			else
				mv = fc;
			if( pcnt )
			{
			       	gv = FIM_INT_PCNT_OF_100(gv,mv);
			}
			if( !mv )
			{
			       	goto ret; 
			}
			if( isfg && ispg )
			{
				goto err;
			}
			//if((!isre) && (!isrj))nf=gv;
			//if(isrj && gv<0 && cf==1){cf=0;}//TODO: this is a bugfix
			if( (!isrj) && gv > 0 )
				gv = gv - 1;// user input is interpreted as 1-based 

			if( !isbl )
				gv = FIM_MOD(gv,mv);

			if( ispg )
			{
				if( isrj )
				{
					np = cp + gv;
					if( isbl )
						np = FIM_MIN(FIM_MAX(np,0),mv-1);
				}
				else
					np = gv;
			}
			else
			{
				np = 0; /* first page -- next file's page count is unknown ! */
				if( isrj )
				{
					nf = cf + gv;
					if( isbl )
						nf = FIM_MIN(FIM_MAX(nf,0),mv-1);
				}
				else
					nf = gv;
			}
			gv = FIM_MOD(gv,mv);
			nf = FIM_MOD(nf,fc);
			np = FIM_MOD(np,pc);

			if(0)
			cout << "goto: "
				<<" s:" << s
				<<" cf:" << cf 
				<<" cp:" << cp 
				<< " nf:" << nf 
				<< " np:" << np 
				<< " gv:" << gv 
				<< " mv:" << mv 
				<< " isrj:"<<isrj
				<< " ispg:"<<ispg
				<< " isfg:"<<isfg
				<< " pcnt:"<<pcnt
				<< " max[pf]:"<<mv
				<<FIM_CNS_NEWLINE;
			if(isre)
			{
				gss.isre = isre;
				gss.src_dir = ((c=='-')?-1:1);

				if( (c == '+' || c == '-' ) && sl == 3 && s[1] == '/' && s[2] == '/' )
					gss.s_str = last_regexp_;
				else
                                {
				        const int sks = (c == '+' || c == '-' ) ? 1 : 0;
					gss.s_str = (string(s).substr(1+sks,sl-2-sks));
                                }
				FIM_PR('.');
				goto ret;
			}
go_jump:
			if( ( nf != cf ) || ( np != cp ) )
			{	
				gss.nf = nf;
				gss.np = np;
				gss.isfg = isfg;
				gss.ispg = ispg;
				goto ret;
			}
		}
ret:
		errmsg = FIM_CNS_EMPTY_RESULT;
err:
		gss.errmsg = errmsg;
		FIM_PR('.');
		return gss;
	} /* goto_image_compute */

	fim::string Browser::goto_image_internal(const fim_char_t *s,fim_xflags_t xflags)
	{
		// Note: further cleanup and simplifications needed.
		const int cf = flist_.cf(), cp = getGlobalIntVariable(FIM_VID_PAGE);
		fim_int nf = cf, np = cp;

		if (s && *s && regexp_match(s,FIM_GOTO_MULTI_JUMP_REGEX,1))
		{
			const char * o = s;
			const char * n;

			do
			{
				n = o;
				while (*n && !isdigit(*n)) // +-
					++n;
				while (*n &&  isdigit(*n)) // number
					++n;
				while (*n && *n=='%')
					++n;

				const bool autojmp = (tolower(*n)!='f' && tolower(*n)!='p');
				const char jmpchar = ( autojmp && n_files() == 1 ) ? 'p' : FIM_SYM_CHAR_NUL;

				while (*n && (tolower(*n)=='f' || tolower(*n)=='p')) // [fp]
					++n;

				const auto ss = string(o).substr(0,n-o) + jmpchar;
				const fim_goto_spec_t gss = goto_image_compute (ss.c_str(),xflags);

				if (cf != gss.nf && gss.nf != FIM_CNS_NO_JUMP_FILE_INDEX)
					nf = gss.nf;
				if (cp != gss.np && gss.np != FIM_CNS_NO_JUMP_PAGE_INDEX)
					np = gss.np;
				o = n;
			}
			while (*o);
		}
		else
		{
			const fim_goto_spec_t gss = goto_image_compute (s,xflags);

			if (gss.errval)
				return FIM_CNS_EMPTY_RESULT;

			if (gss.s_str.size())
				return regexp_goto({gss.s_str},gss.src_dir);
			nf = gss.nf;
			np = gss.np;
		}

		if ( nf!=FIM_CNS_NO_JUMP_FILE_INDEX || np!=FIM_CNS_NO_JUMP_PAGE_INDEX )
		{
			if( ( nf != cf ) || ( np != cp ) )
			{	
				const fim_int oap = getGlobalFloatVariable(FIM_VID_PAGE);
				const fim_int ap = oap; // FIXME: page one reverts to zero; this needs to be fixed elsewhere
				const fim::string ncf = current();
				if(!(xflags&FIM_X_NOAUTOCMD))
				{ FIM_AUTOCMD_EXEC(FIM_ACM_PREGOTO,ncf); }

				if( nf != cf )
					goto_image(nf,true);

				if( np != cp )
					if(viewport())
						// viewport()->img_goto_page(np); // modifying the current() image_ breaks a subsequent freeCachedImage originating from a reload()
						setGlobalVariable(FIM_VID_PAGE,np); // this instead, is only symbolic and it's enough for now
				setGlobalVariable(FIM_VID_LASTPAGEINDEX,ap);
				if(!(xflags&FIM_X_NOAUTOCMD))
				{ FIM_AUTOCMD_EXEC(FIM_ACM_POSTGOTO,ncf); }
			}
		}
		return FIM_CNS_EMPTY_RESULT;
	} /* goto_image_internal */

	fim_stat_t fim_get_stat(const fim_fn_t& fn, bool * dopushp);

	fim::string Browser::do_filter(const args_t& args, MatchMode rm, bool negative, enum FilterAction faction)
	{
		/*
		 * TODO: introduce flags for negative (instead of a bool).
		 * TODO: message like 'marked xxx files' ...
		 */
		fim::string result;
		const bool wom = (flist_.size() > FIM_CNS_ENOUGH_FILES_TO_WARN );
		fim_int marked = 0;
		fim_bitset_t lbs(flist_.size()); // limit bitset, used for mark / unmark / delete / etc
		// fim_fms_t dt;

		FIM_PR('*');
		// std::cout << "match mode:" << rm << "  negation:" << negative << "  faction:" << faction << "\n"; // TODO: a debug mode shall enable such printouts.

		if ( args.size() > 1 && rm == ListIdxMatch )
		{
			size_t min_idx=0,max_idx=FIM_MAX_VALUE_FOR_TYPE(size_t);
			const std::string ss(args[1]);
			std::istringstream is(ss);

			if(is.peek()!='-') // MIN-
			{
				if(!isdigit(is.peek()))
				{
					result = "Bad MINIDX[-MAXIDX] expression ! MINIDX or MAXIDX should be a number.";
					goto nop;
				}
				is >> min_idx;
				if(tolower(is.peek())=='k')
					is.ignore(1), min_idx *= FIM_CNS_Ki;
				while(is.peek() != -1 && is.peek() != '-')
					is.ignore(1);
				if(is.peek() == -1)
				{
					max_idx=min_idx;
					goto parsed_idx;
				}
			}

			if(is.peek()=='-') // -MAX
			{
				is.ignore(1);
				is >> max_idx;
				if(tolower(is.peek())=='k')
					is.ignore(1), max_idx *= FIM_CNS_Ki;
			}
parsed_idx:
			if(min_idx>max_idx)
				std::swap(min_idx,max_idx);
		
			if(min_idx>=flist_.size() || max_idx < 1)
			{
				if(wom)
					commandConsole_.set_status_bar("requested index range is wrong (not so many files...)", "*");
				goto nop;
			}

			min_idx=FIM_MIN(FIM_MAX(min_idx,1U),flist_.size());
			max_idx=FIM_MIN(FIM_MAX(max_idx,1U),flist_.size());

			// std::cout << "Limiting index between " << min_idx << " and " << max_idx << " .\n";

			if(min_idx==1 && max_idx>=flist_.size())
			{
				if(wom)
					commandConsole_.set_status_bar("requested index limit range covers entire list.", "*");
				goto nop;
			}

			if(wom)
				commandConsole_.set_status_bar("limiting to list index range...", "*");

			/* not efficient but conceptually clean in this context */
			for(size_t fi=min_idx;fi<=max_idx;++fi)
				lbs.set(fi-1);
			negative = !negative;
			goto rfrsh;
		}

		if ( rm == TimeMatch || rm == SizeMatch )
		{
#if FIM_WANT_FLIST_STAT 
			off_t min_size=0,max_size=FIM_MAX_VALUE_FOR_TYPE(off_t);
			time_t min_mtime=0,max_mtime=FIM_MAX_VALUE_FOR_TYPE(time_t);
			const std::string ss(args[args.size()>1?1:0]); // if 0 we'll ignore this
			std::istringstream is(ss);

			if ( rm == SizeMatch )
			{
				if(args.size()==1)
				{
					const fim_stat_t fss = fim_get_stat(current(),FIM_NULL);
					if( ( min_size=max_size=fss.st_size ) == 0 ) 
						goto nop;
					goto parsed_limits;
				}
				
				if(is.peek()!='-') // MIN-
				{
					if(!isdigit(is.peek()))
					{
						result = "Bad MINSIZE[-MAXSIZE] expression ! MINSIZE or MAXSIZE should be a number followed by K or M.";
						goto nop;
					}
					is >> min_size;
					if(tolower(is.peek())=='k')
						is.ignore(1), min_size *= FIM_CNS_K;
					else
					if(tolower(is.peek())=='m')
						is.ignore(1), min_size *= FIM_CNS_M;
					while(is.peek() != -1 && is.peek() != '-')
						is.ignore(1);
					if(is.peek() == -1)
					{
						max_size=min_size;
						goto parsed_limits;
					}
				}

				if(is.peek()=='-') // -MAX
				{
					is.ignore(1);
					is >> max_size;
					if(tolower(is.peek())=='k')
						is.ignore(1), max_size *= FIM_CNS_K;
					else
					if(tolower(is.peek())=='m')
						is.ignore(1), max_size *= FIM_CNS_M;
				}
			}

			if ( rm == TimeMatch )
			{
				if(args.size()==1)
				{
					const fim_stat_t fss = fim_get_stat(current(),FIM_NULL);
					if( ( min_mtime=max_mtime=fss.st_mtime ) == 0 ) 
						goto nop;
					min_mtime-=60*60*24;
					max_mtime+=60*60*24;
					goto parsed_limits;
				}
				
				if(strchr(args[1].c_str(),'/'))
				{
					// DD/MM/YYYY format
					min_mtime=0;

					if(isdigit(is.peek()))
					{
						struct tm min_mtime_str;
						fim_bzero(&min_mtime_str,sizeof(min_mtime_str));

						is >> min_mtime_str.tm_mday;
						if(is.peek() != '/')
							goto errpd;
						is.ignore(1);
						is >> min_mtime_str.tm_mon;
						min_mtime_str.tm_mon-=1;
						if(is.peek() != '/')
							goto errpd;
						is.ignore(1);
						is >> min_mtime_str.tm_year;
						min_mtime_str.tm_year-=1900;

						min_mtime=mktime(&min_mtime_str); // TODO: add check if 4-bytes time_t and after 20380119
					}
					else
						; // -MAX

					if(is.peek() == -1)
						max_mtime = min_mtime;

					if(is.peek() != '-')
						goto errpd;
					is.ignore(1); // eat '-'

					if(!isdigit(is.peek()))
						goto errpd;
					else
					{
						struct tm max_mtime_str;
						fim_bzero(&max_mtime_str,sizeof(max_mtime_str));

						is >> max_mtime_str.tm_mday;
						if(is.peek() != '/')
							goto errpd;
						is.ignore(1);
						is >> max_mtime_str.tm_mon;
						max_mtime_str.tm_mon-=1;
						if(is.peek() != '/')
							goto errpd;
						is.ignore(1);
						is >> max_mtime_str.tm_year;
						max_mtime_str.tm_year-=1900;

						max_mtime=mktime(&max_mtime_str); // TODO: add check if 4-bytes time_t and after 20380119
					}
					// std::cout << min_mtime << " .. "  << max_mtime << "\n";

					goto parsed_limits;
				}

				// shall proof this code once more for corner cases 
				if(is.peek()!='-') // MIN-
				{
					if(!isdigit(is.peek()))
					{
						result = "Bad MINTIME[-MAXTIME] expression ! MINTIME or MAXTIME should be a number.";
						goto nop;
					}
					is >> min_mtime;
					while(is.peek() != -1 && is.peek() != '-')
						is.ignore(1);
					if(is.peek() == -1)
					{
						max_mtime=min_mtime;
						goto parsed_limits;
					}
				}

				if(is.peek()=='-') // -MAX
				{
					is.ignore(1);
					is >> max_mtime;
				}
			}
parsed_limits:
			if(min_size>max_size)
				std::swap(min_size,max_size);
			if(min_mtime>max_mtime)
				std::swap(min_mtime,max_mtime);

			// std::cout << "Limiting size between " << min_size << " and " << max_size << " bytes.\n";
			// std::cout << "Limiting time between " << min_mtime << " and " << max_mtime << " .\n";

			if(wom)
				 commandConsole_.set_status_bar("getting stat() info...", "*");
			flist_.get_stat();

			if ( rm == SizeMatch )
			{
				if(wom)
					commandConsole_.set_status_bar("limiting to a size...", "*");
				for(size_t fi=0;fi<flist_.size();++fi)
					if ( ! FIM_WITHIN ( min_size, flist_[fi].stat_.st_size, max_size ) )
						lbs.set(fi);
			}
			if ( rm == TimeMatch )
			{
				if(wom)
					commandConsole_.set_status_bar("limiting to a timespan...", "*");
				for(size_t fi=0;fi<flist_.size();++fi)
					if ( ! FIM_WITHIN ( min_mtime, flist_[fi].stat_.st_mtime, max_mtime ) )
						lbs.set(fi);
			}
#else /* FIM_WANT_FLIST_STAT */
#endif /* FIM_WANT_FLIST_STAT */
			goto rfrsh;
	errpd:
			goto nop;
		}
#if FIM_WANT_LIMIT_DUPBN
		if ( rm == DupFileNameMatch || rm == UniqFileNameMatch || rm == FirstFileNameMatch || rm == LastFileNameMatch)
		{
			flist_t slist = flist_;

			slist._sort(FIM_SYM_SORT_BN);
			if(wom)
			{
				const char * msg = "";
				switch( rm )
				{
					case(  DupFileNameMatch): msg = "limiting to duplicate base filenames..."; break;
					case( UniqFileNameMatch): msg = "limiting to unique base filenames..."; break;
					case(FirstFileNameMatch): msg = "limiting to first base filenames..."; break;
					case( LastFileNameMatch): msg = "limiting to last base filenames..."; break;
					default: msg = ""; /* not foreseen in this branch */
				}
			       	commandConsole_.set_status_bar(msg, "*");
			}

			/* as an improvement, might use a std::map<file name type, bit>, rather than this */
			for(size_t i=0;i<slist.size();++i)
			{
				size_t j=i,ii=0,jj=0;

				while(j+1<slist.size() && 0 == 
					strcmp(fim_basename_of(slist[i]),fim_basename_of(slist[j+1])) )
					++j;
				// j+1-i same basenames
				if ( rm == DupFileNameMatch )
				{
					if(i==j)
						ii=i,jj=i; // no dups: no results
					else
						ii=i,jj=j+1; // all dups: all results
				}
				if ( rm == UniqFileNameMatch )
				{
					if(i==j)
						ii=i,jj=i+1; // uniq; a result
					else
						ii=i,jj=i; // there are dups: no results
				}
				if ( rm == FirstFileNameMatch)
				{
					ii=i,jj=i+1; // first is result
				}
				if ( rm == LastFileNameMatch)
				{
					ii=j,jj=j+1; // last is result
				}
				for(i=ii;i<jj;++i)
				{
					fim_int fi = find_file_index(slist[i]);
					if(fi>=0)
						lbs.set(fi);
				}
				i=j;
			}
			negative = !negative;
			goto rfrsh;
		}
#endif /* FIM_WANT_LIMIT_DUPBN */
#if FIM_WANT_FILENAME_MARK_AND_DUMP
		if ( rm == MarkedMatch )
		{
			if(wom)
				commandConsole_.set_status_bar("limiting marked...", "*");
			for(size_t i=0;i<flist_.size();++i)
				if( cc.isMarkedFile(flist_[i]) == negative )
					lbs.set(i);
			goto rfrsh;
		}
#endif /* FIM_WANT_FILENAME_MARK_AND_DUMP */
		if( flist_.size() < 1 )
		{
			result = "the files list is empty\n";
			goto nop;
		}
		if(args.size()>0)
		{
			args_t rlist = args;	//the remove list
			/*
			 * the list is unsorted. it may contain duplicates
			 * if this software will have success, we will have indices here :)
			 * sort(rlist.begin(),rlist.end());...
			 */
			//fim_int lf = flist_.size();
#if FIM_WANT_PIC_LVDN
			if ( rm == VarMatch )
				if(rlist.size() < 2)
					rm = CmtMatch;
#else
			if ( rm == VarMatch )
				rm = FullFileNameMatch;
#endif /* FIM_WANT_PIC_LVDN */
			// if(wom) commandConsole_.set_status_bar("limiting matching...", "*");
			if(wom)
			{
				fim::string msg = "";
				switch( rm )
				{
					case(FullFileNameMatch   ): msg = "limiting to full filename match..."; break;
					case(PartialFileNameMatch): msg = "limiting to partial filename match..."; break;
					case(VarMatch            ): msg = string("limiting variable \"") + string(rlist[0]) + string("\" matching value \"") + string(rlist[1]) + string("\" ..."); break;
					case(CmtMatch            ): msg = "limiting to comment match..."; break;
					default: msg = ""; /* not foreseen in this branch */
				}
			       	commandConsole_.set_status_bar(msg, "*");
			}

			// dt = - getmilliseconds();
			for(size_t r=0;r<rlist.size();++r)
			for(size_t i=0;i<flist_.size();++i)
			{
				bool match = false;
#if HAVE_FNMATCH_H && 0
				match |= ( rm == FullFileNameMatch && ( ( 0 == fnmatch(rlist[r].c_str(), flist_[i].c_str(),0)  ) != negative ) );
#else /* HAVE_FNMATCH_H */
				match |= ( rm == FullFileNameMatch && ( ( flist_[i] == rlist[r] ) != negative ) );
#endif /* HAVE_FNMATCH_H */
				match |= ( rm == PartialFileNameMatch && ( ( flist_[i].re_match(rlist[r].c_str()) ) != negative ) );
#if FIM_WANT_PIC_LVDN
				match |= ( rm == VarMatch     && ( ( cc.id_.vd_[fim_basename_of(flist_[i])].getStringVariable(rlist[0]) == rlist[1] ) != negative ) );
				match |= ( rm == CmtMatch     && ( ( string(cc.id_    [fim_basename_of(flist_[i])]).re_match(rlist[0]) ) != negative ) );
#endif /* FIM_WANT_PIC_LVDN */
				if( match )
					lbs.set(i); // TODO: further cycles on r are unnecessary after match.
			}
			// dt+ =  getmilliseconds();
			// std::cout << fim::string("    limiting took ") << dt << " ms" << std::endl;

	//		if(lf != flist_.size() )
	//			cout << "limited to " << int(flist_.size()) << " files, excluding " << int(lf - flist_.size()) << " files." <<FIM_CNS_NEWLINE;
			if(negative)
			       	negative = !negative;
			goto rfrsh;
		}
		else
		{
			/* removes the current file from the list */
			result = pop_current();
			goto nop;
		}
rfrsh:
		if (negative)
			lbs.negate();
		// dt = - getmilliseconds();
		if (lbs.any())
		{
			const size_t tsize = lbs.size();
			const size_t cnt = (faction == Delete) ? tsize - lbs.count() : lbs.count();


			std::ostringstream msg;
		       	msg << "Selected " << cnt << " files out of " <<  tsize << ".\n";
			result = msg.str();

			if(faction == Delete)
			{
				if(cnt < tsize/2) // a very rough but effective first optimization
					flist_ = flist_.copy_from_bitset(lbs);
				else
					flist_.erase_at_bitset(lbs);
			}
			else /* Mark / Unmark */
			{
				for(size_t pos=0;pos<tsize;++pos)
					if(lbs.at(pos))
						marked += cc.markFile(flist_[pos],(faction == Mark),false);
			}
		}
		// dt += getmilliseconds();
		// std::cout << fim::string("bit limiting took ") << dt << " ms" << std::endl;

		//if ((faction != Delete) && marked)
		//	cout << ( faction == Mark ? "  Marked " : "Unmarked "  ) << marked << " files\n";

		setGlobalVariable(FIM_VID_FILEINDEX,current_image());
		setGlobalVariable(FIM_VID_FILELISTLEN,n_files());
		goto nop;
		result = FIM_CNS_EMPTY_RESULT;
nop:
		FIM_PR('.');
		return result;
	} /* do_filter */

	fim_cxr Browser::scrollforward(const args_t& args)
	{
		if(c_getImage() && viewport())
		{
			const fim_int vss = cc.getIntVariable(FIM_VID_SKIP_SCROLL);
			const fim_coo_t approx_fraction = ( vss == 1 ? 16 : vss );

			if(viewport()->onRight(approx_fraction) && viewport()->onBottom(approx_fraction))
				next(),
				fcmd_align({"top"});
			else
			if(viewport()->onRight(approx_fraction))
			{
				viewport()->pan("down",FIM_CNS_SCROLL_DEFAULT);
				while(!(viewport()->onLeft()))
					viewport()->pan("left",FIM_CNS_SCROLL_DEFAULT);
			}
			else
			       	viewport()->pan("right",FIM_CNS_SCROLL_DEFAULT);
		}
		else
		       	next(1);
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_cxr Browser::scrolldown(const args_t& args)
	{
		if(c_getImage() && viewport())
		{
			if(viewport()->onBottom())
				next(),
				fcmd_align({"top"});
			else
				viewport()->pan("down",FIM_CNS_SCROLL_DEFAULT);
		}
		else
		       	next(1);
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_cxr Browser::fcmd_scroll(const args_t& args)
	{
		FIM_PR('*');
		FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREPAN,current())
		if(args.size()>0 && args[0]=="forward")
			scrollforward(args);
		else
			scrolldown(args);
		FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTPAN)
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_cxr Browser::fcmd_info(const args_t& args)
	{
		/*
		 *	short information in status-line format
		 */
		fim::string r = current();
		FIM_PR('*');

		if(c_getImage())
			r += c_getImage()->getInfo();
		else
			r += " (unloaded)";
		FIM_PR('.');
		return r;
	}

#if FIM_WANT_OBSOLETE
	fim::string Browser::info(void)
	{
		return fcmd_info(args_t(0));
	}
#endif /* FIM_WANT_OBSOLETE */

	fim_cxr Browser::fcmd_rotate(const args_t& args)
	{
		fim_angle_t angle; // degrees
		FIM_PR('*');

		if( args.size() == 0 )
			angle = FIM_CNS_ANGLE_ONE;
		else
			angle = fim_atof(args[0].c_str());
		if( angle == FIM_CNS_ANGLE_ZERO)
			goto ret;

		if(c_getImage())
		{
			//angle = getGlobalFloatVariable(FIM_VID_ANGLE);
//			FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREROTATE,current())
			FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PRESCALE,current())
			if(viewport())
				viewport()->img_rotate(angle);
			FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTSCALE)
//			FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTROTATE)
		}
ret:
		FIM_PR('.');
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_cxr Browser::fcmd_align(const args_t& args)
	{
		fim::string result = FIM_CMD_HELP_ALIGN;
		FIM_PR('*');
		if( args.size() < 1 )
			goto err;
		if( !args[0].c_str() || !args[0].re_match("^(bottom|top|left|right|center|info)") )
			goto err;

		if(args[0].re_match("info"))
		{
			ViewportState viewportState = *viewport();
			std::ostringstream oss;
			oss << "alignment info:";
			oss << " steps:" << viewportState.steps_;
			oss << " hsteps:" << viewportState.hsteps_;
			oss << " vsteps:" << viewportState.vsteps_;
			oss << " top:" << viewportState.top_;
			oss << " left:" << viewportState.left_;
			oss << " panned:" << viewportState.panned_;
			result = oss.str();
		}
		else
		if( c_getImage() )
		{
			FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PREPAN,current())
			if(c_getImage() && viewport())
			{
				if(args[0].re_match("top"))
					viewport()->align('t');
				if(args[0].re_match("bottom"))
					viewport()->align('b');
				if(args[0].re_match("left"))
					viewport()->align('l');
				if(args[0].re_match("right"))
					viewport()->align('r');
				if(args[0].re_match("center"))
					viewport()->align('c');
			}
			FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTPAN)
			result = FIM_CNS_EMPTY_RESULT;
		}
err:
		FIM_PR('.');
		return result;
	}

	const Image* Browser::c_getImage(void)const
	{
		const Image* image = FIM_NULL;

		if( viewport() )
			image = viewport()->c_getImage();
		return image;
	}

	Viewport* Browser::viewport(void)const
	{
		return (commandConsole_.current_viewport());
	}

	fim_fn_t Browser::current(void)const
	{
		if( empty_file_list() )
			return nofile_;
	       	return flist_[flist_.cf()];
	}

	int Browser::empty_file_list(void)const
	{
		return flist_.size() == 0;
	}

	fim::string Browser::do_push(const args_t& args)
	{
		/* push to image list */
		for(size_t i=0;i<args.size();++i)
		{
#ifdef FIM_SMART_COMPLETION
			/* This patch allows first filename argument unquoted. */
			/* However, fim syntax uses space to separate arguments; so the following limitation here.  */
			fim::string ss = args[i];
			ss.substitute(" +$","");
			push_path(ss);
#else /* FIM_SMART_COMPLETION */
			push_path(args[i]);
#endif /* FIM_SMART_COMPLETION */
		}
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_int Browser::current_image(void)const
	{
		/* count from 1 */
		return flist_.cf() + 1;
	}

	fim_int Browser::n_pages(void)const
	{
		fim_int pi = 0;
#if !FIM_WANT_BDI
		if( c_getImage() )
#endif	/* FIM_WANT_BDI */
			pi = c_getImage()->n_pages();
		return pi;
	}

	size_t Browser::byte_size(void)const
	{
		size_t bs = 0;

		bs += cache_.byte_size();
		bs += sizeof(*this);
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 1
		for (const auto & e : hlist_)
			bs += e.capacity();
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
		/* TODO: this is incomplete ... */
		return bs;
	}

	void Browser::mark_from_list(const args_t& argsc)
	{
		/* TODO: one would like to have better matching options */
		if (argsc.size() )
			this->do_filter(argsc,Browser::PartialFileNameMatch,false,Browser::Mark);
	}

	bool Browser::dump_desc(const fim_fn_t nf, fim_char_t sc, const bool want_all_vars, const bool want_append)const
	{
		/* 
		   Dumps descriptions of the current images list.
		   on no comment does not touch the file.
		 */
#if FIM_WANT_PIC_LBFL
		// TODO: if no nf, then standard output
		std::ostringstream cmtfc; // comment file contents

		for(size_t i=0;i<flist_.size();++i)
		{
			fim::string bof = flist_[i].c_str();
#if FIM_WANT_PIC_LVDN
			if( cc.id_.find(bof) == cc.id_.end() )
				bof = fim_basename_of(flist_[i]);
			if(want_all_vars)
				cmtfc << cc.id_.vd_[bof].get_variables_list(true, true);
#endif /* FIM_WANT_PIC_LVDN */
			cmtfc << bof << sc << cc.id_[bof] << "\n";
#if FIM_WANT_PIC_LVDN
#if 0
			if(want_all_vars)
				cmtfc << cc.id_.vd_[bof].get_variables_list(false,true) << "\n";
#else
			if(want_all_vars)
				cmtfc << "#!fim:!=\n"; /* clean up vars for the next */
#endif
#endif /* FIM_WANT_PIC_LVDN */
		}

		if ( write_to_file(nf,cmtfc.str(),want_append) )
			cout << "Successfully wrote to file " << nf << " .\n";
		else
			goto err;
/*
 		// TODO: this will print info about all of them
		cc.id_.print_descs(std::cout,sc);
*/
		return true;
err:
#else /* FIM_WANT_PIC_LBFL */
		/* ... */
#endif /* FIM_WANT_PIC_LBFL */
		return false;
	}

#if FIM_WANT_BENCHMARKS
	fim_int Browser::get_n_qbenchmarks(void)const
	{
		return 2;
	}

	void Browser::quickbench_init(fim_int qbi)
	{
		string msg;
		
		switch(qbi)
		{
			case 0:
				msg="fim browser push check";
				std::cout << msg << ": " << "please be patient\n";
			break;
			case 1:
				msg="fim browser pop check";
				std::cout << msg << ": " << "please be patient\n";
			break;
		}
	}

	void Browser::quickbench_finalize(fim_int qbi)
	{
	}

	void Browser::quickbench(fim_int qbi)
	{
		static fim_int ci = 0;

		switch(qbi)
		{
			case 0:
				push_path(string(ci)+string(".jpg"));
				ci++;
			break;
			case 1:
				ci--;
				push_path(string(ci)+string(".jpg"));
			break;
		}
	}

	string Browser::get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const
	{
		std::ostringstream oss;
		switch(qbi)
		{
			case 0:
				oss << "fim browser push check" << ": " << string((float)(((fim_fms_t)qbtimes)/((qbttime)*1.e-3))) << " push_path()/s\n";
			break;
			case 1:
				oss << "fim browser pop check" << ": " << ((fim_fms_t)qbtimes)/((qbttime)*1.e-3) << " pop()/s\n";
			break;
		}
		return oss.str();
	}
#endif /* FIM_WANT_BENCHMARKS */

	bool Browser::filechanged(void)
	{
#if FIM_WANT_RELOAD_ON_FILE_CHANGE
#if FIM_WANT_FLIST_STAT 
		const fim_fn_t fn = current();

		if( fn.size() && fn != FIM_STDIN_IMAGE_NAME )
		{
			const int cn = current_n();
			const fim_stat_t nfs = fim_get_stat(fn, NULL);
			const fim_stat_t ofs = flist_[cn].stat_;

			if( ofs.st_ctime != nfs.st_ctime || ofs.st_mtime != nfs.st_mtime )
			{
				flist_[cn].stat_ =  nfs;
//craiger zero-byte patch       if( ofs.st_ctime && ofs.st_mtime )
//					return true;
                                if( nfs.st_size >= FIM_MIN_FILESIZE_FOR_CHANGED )
                                        if( ofs.st_ctime && ofs.st_mtime )
                                                return true;
			}
		}
#endif /* FIM_WANT_FLIST_STAT */
#endif /* FIM_WANT_RELOAD_ON_FILE_CHANGE */
		return false;
	}
	fim_stat_t fim_get_stat(const fim_fn_t& fn, bool * dopushp)
	{
		bool dopush = false;
		fim_stat_t stat_s;
#if FIM_WANT_FLIST_STAT
		if(-1==stat(fn.c_str(),&stat_s))
			goto ret;
		if( S_ISDIR(stat_s.st_mode))
			goto ret;
		if( stat_s.st_size == 0 )
			goto ret;
#endif /* FIM_WANT_FLIST_STAT */
		dopush = true;
ret:
		if( dopushp )
		       *dopushp = dopush;
		return stat_s; // FIXME: what about failure ?
	}

	void flist_t::get_stat(void)
	{
#if FIM_WANT_FLIST_STAT
		// TODO: this is expensive: might print a message here.
		for(flist_t::iterator fit=begin();fit!=end();++fit)
			fit->stat_ = fim_get_stat(*fit,FIM_NULL);
#endif /* FIM_WANT_FLIST_STAT */
	}

	flist_t::flist_t(const args_t& a):cf_(0)
       	{
		/* FIXME: unused for now */
		this->reserve(a.size());
		for(args_t::const_iterator fit=a.begin();fit!=a.end();++fit)
		{
			bool dopush;
			const fim_stat_t stat_s = fim_get_stat(*fit,&dopush);
			if(dopush)
				push_back(fim::fle_t(*fit,stat_s));
		}
		this->shrink_to_fit();
	}

	const fim::string flist_t::pop(const fim::string& filename, bool advance)
	{
		fim::string s;

		if( filename != FIM_CNS_EMPTY_STRING )
		{
			const size_t idx = std::find(this->begin(),this->end(),filename) - this->begin();
			this->erase(std::remove_if(this->begin(),this->end(),[&](const fim::fle_t&fl)->bool{return fim::string(fl)==filename;}),this->end());
			if (idx < cf_)
				cf_--;
		}
		else
		if( size() )
		{
			assert(cf_>=0);
			s = (*this)[cf_];
			this->erase( this->begin() + cf_ );
			if(cf_ && ! (advance && cf_ < size() ) )
			       	cf_--;
		}
		return s;
	}

	void flist_t::_unique()
	{
		// this only makes sense if the list is sorted.
		this->erase(std::unique(this->begin(), this->end()), this->end());
		adj_cf();
	}

	void flist_t::_set_union(const flist_t & clist)
	{
					flist_t mlist;
					this->_sort(FIM_SYM_SORT_FN);
					mlist.reserve(this->size()+clist.size());
					std::set_union(clist.begin(),clist.end(),this->begin(),this->end(),std::back_inserter(mlist));
					mlist.shrink_to_fit();
					this->assign(mlist.begin(),mlist.end());
	}

	void flist_t::_set_difference_from(const flist_t & clist)
	{
					flist_t mlist;
					this->_sort(FIM_SYM_SORT_FN);
					mlist.reserve(this->size()+clist.size());
					std::set_difference(clist.begin(),clist.end(),this->begin(),this->end(),std::back_inserter(mlist));
					mlist.shrink_to_fit();
					this->assign(mlist.begin(),mlist.end());
	}

void flist_t::erase_at_bitset(const fim_bitset_t& bs, fim_bool_t negative)
{
	size_t ecount = 0;
	const auto bit=this->begin();
	const auto eit=this->end();
	for(auto fit=bit;fit!=eit;++fit)
		if(bs.at(fit-bit) != negative)
			this->erase(fit-ecount),ecount++;
	cf_=size()?std::min(size()-1,cf_):0;// TODO: need tests and connection with adj_cf and copy_from_bitset
}
	fim_bool_t flist_t::pop_current(void)
	{
		if( this->size() <= 0 )
			return false;
		this->erase( this->begin() + cf() );
		return true;
	}

flist_t flist_t::copy_from_bitset(const fim_bitset_t& bs, fim_bool_t positive) const
{
	flist_t nlist;
	const auto bit=this->cbegin();
	const auto eit=this->cend();
	for(auto fit=bit;fit!=eit;++fit)
		if(bs.at(fit-bit) != positive )
			nlist.emplace_back(*fit);// this might spare constructor
			//nlist.push_back(*fit);
	return nlist;
}

	//std::string fle_t::operator(){return fn;}
	fle_t::fle_t(void) { }
	fle_t::fle_t(const fim_fn_t& s):fim_fn_t(s)
	{
#if FIM_WANT_FLIST_STAT 
		fim_bzero(&stat_,sizeof(stat_));
#endif /* FIM_WANT_FLIST_STAT */
	}
	fle_t::fle_t(const fim_fn_t& s, const fim_stat_t & ss):fim_fn_t(s)
#if FIM_WANT_FLIST_STAT 
	,stat_(ss)
#endif /* FIM_WANT_FLIST_STAT */
	{ }

struct FimBaseNameSorter
{
	bool operator() (const fim_fn_t& lfn, const fim_fn_t& rfn)
	{ 
		const char * ls = lfn.c_str();
		const char * rs = rfn.c_str();
		int scr = 0;

		if(ls && rs)
			scr = (strcmp(fim_basename_of(ls),fim_basename_of(rs)));
		return (scr < 0);
	}
} fimBaseNameSorter;

struct FimByVarSorter
{
	std::string id;
	bool operator() (const fim_fn_t& lfn, const fim_fn_t& rfn)
	{ 
		const char * ls = lfn.c_str();
		const char * rs = rfn.c_str();
		int scr = 0;

		const std::string lsv = cc.id_.vd_[fim_basename_of(ls)].getStringVariable(id);
		const std::string rsv = cc.id_.vd_[fim_basename_of(rs)].getStringVariable(id);

		scr = (strcmp(lsv.c_str(),rsv.c_str()));
		return (scr < 0);
	}
};

#if FIM_WANT_SORT_BY_STAT_INFO
#if FIM_WANT_FLIST_STAT 
struct FimSizeSorter
{
	bool operator() (const fim::fle_t & lfn, const fim::fle_t & rfn)
	{ 
		return (lfn.stat_.st_size < rfn.stat_.st_size);
	}
}fimSizeSorter;
struct FimDateSorter
{
	bool operator() (const fim::fle_t & lfn, const fim::fle_t & rfn)
	{ 
		return (lfn.stat_.st_mtime < rfn.stat_.st_mtime);
	}
}fimDateSorter;
#else /* FIM_WANT_FLIST_STAT */
struct FimDateSorter
{
	bool operator() (const fim_fn_t& lfn, const fim_fn_t& rfn)
	{ 
		struct stat lstat_s;
		struct stat rstat_s;
		stat(lfn.c_str(),&lstat_s);
		stat(rfn.c_str(),&rstat_s);
		return (lstat_s.st_mtime < rstat_s.st_mtime);
	}
}fimDateSorter;
#endif /* FIM_WANT_FLIST_STAT */
#endif /* FIM_WANT_SORT_BY_STAT_INFO */

	void flist_t::_sort(const fim_char_t sc, const char*id)
	{
		/* TODO: can cache sorting status, would make e.g. Browser::do_filter more efficient */

		if(sc==FIM_SYM_SORT_FN)
			std::sort(this->begin(),this->end());
		if(sc==FIM_SYM_SORT_BN)
			std::sort(this->begin(),this->end(),fimBaseNameSorter);
		if(sc==FIM_SYM_SORT_BV)
		{
			FimByVarSorter fimByVarSorter;
			fimByVarSorter.id=id;
			std::sort(this->begin(),this->end(),fimByVarSorter);
		}
#if FIM_WANT_SORT_BY_STAT_INFO
		if(sc==FIM_SYM_SORT_MD)
		{
			this->get_stat();
			std::sort(this->begin(),this->end(),fimDateSorter);
		}
		if(sc==FIM_SYM_SORT_SZ)
		{
			this->get_stat();
			std::sort(this->begin(),this->end(),fimSizeSorter);
		}
#endif /* FIM_WANT_SORT_BY_STAT_INFO */
	}
} /* namespace fim */
