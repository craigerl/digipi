/* $LastChangedDate: 2024-05-11 14:46:02 +0200 (Sat, 11 May 2024) $ */
/*
 Browser.h : Image browser header file

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
#ifndef FIM_BROWSER_H
#define FIM_BROWSER_H
#include "fim.h"
#include <algorithm> // std::count
#include <memory> // std::unique_ptr

#define FIM_FLAG_NONREC 0 /* let this be zero */
#define FIM_FLAG_DEFAULT FIM_FLAG_NONREC
#define FIM_FLAG_PUSH_REC 1
#define FIM_FLAG_PUSH_BACKGROUND 2
#define FIM_FLAG_PUSH_ONE 4 /* one is enough */
#define FIM_FLAG_PUSH_HIDDEN 8 /* */
#define FIM_FLAG_PUSH_ALLOW_DUPS 16 /* */
#define FIM_FLAG_PUSH_FILE_NO_CHECK 32 /* inner one, to save a few stat's; might better cache 'stat()' outpout instead.. */
#define FIM_FLAG_DEL(V,F)    (V) &= ~(F)
#define FIM_WANT_LIMIT_DUPBN 1 /* limit to duplicate base name */

namespace fim
{
	extern CommandConsole cc;

#if FIM_WANT_FLIST_STAT 
typedef struct stat fim_stat_t;
#else /* FIM_WANT_FLIST_STAT */
typedef int fim_stat_t;
#endif /* FIM_WANT_FLIST_STAT */

class fle_t FIM_FINAL : public fim_fn_t /* file list element */
{
       	public:
#if FIM_WANT_FLIST_STAT 
       	fim_stat_t stat_;
#endif /* FIM_WANT_FLIST_STAT */
	fle_t(const fim_fn_t& s);
	fle_t(const fim_fn_t& s, const fim_stat_t &ss);
	fle_t();
};

class fim_bitset_t FIM_FINAL :public std::vector<fim_bool_t>
{
	public:
	fim_bitset_t(size_t n):std::vector<fim_bool_t>(n,false){}
	size_t count(void)const{return std::count(this->begin(),this->end(),true);}
	fim_bool_t any(void)const{return count()!=0;}
	void set(size_t pos, fim_bool_t value = true){ (*this)[pos]=value; }
	void negate(void){ for(auto bit : (*this)) bit = !bit;  }
}; /* fim_bitset_t */

/*
std::ostream& operator << (std::ostream& os, const fim_bitset_t& bs)
{
	// FIXME: temporarily here !
	os << bs.size() << ":";
	for(size_t i=0;i<bs.size();++i)
		os << bs.at(i) ? 1 : 0;
	os << "\n";
	return os;
} */

class flist_t FIM_FINAL : public std::vector<fim::fle_t>
{
	private:
	size_type cf_;
	public:
	flist_t(void):cf_(0){}
	flist_t(const args_t& a);
	void _sort(const fim_char_t sc, const char*id="");
	void _unique();
	size_type cf(void)const{return FIM_MAX(cf_,0U);}
	fim_bool_t pop_current(void);
	void erase_at_bitset(const fim_bitset_t& bs, fim_bool_t negative = false);
	flist_t copy_from_bitset(const fim_bitset_t& bs, fim_bool_t positive = true) const;
	void _set_difference_from(const flist_t & clist);
	void _set_union(const flist_t & clist);
	void get_stat(void);
	void set_cf(size_type cf){cf_=FIM_MOD(cf,size());}
	const fim::string pop(const fim::string& filename, bool advance);
	private:
	void adj_cf(void){cf_ = size() ? size()-1:0; }
};

struct fim_goto_spec_t
{
	fim_err_t errval{}; // need to simplify this: too much state
	fim::string errmsg{FIM_CNS_EMPTY_STRING};
	fim::string s_str{};
	fim_int src_dir{0};
	fim_int nf{FIM_CNS_NO_JUMP_FILE_INDEX}, np{FIM_CNS_NO_JUMP_PAGE_INDEX};
	bool isfg{false};
	bool ispg{false};
	bool isre{false};
};

class Browser FIM_FINAL 
#ifdef FIM_NAMESPACES
:public Namespace
#if FIM_WANT_BENCHMARKS
,public Benchmarkable
#endif /* FIM_WANT_BENCHMARKS */
#else /* FIM_NAMESPACES */
 public Benchmarkable
#if FIM_WANT_BENCHMARKS
	: public Benchmarkable,
#endif /* FIM_WANT_BENCHMARKS */
#endif /* FIM_NAMESPACES */
{
	private:
	enum MatchMode{ FullFileNameMatch, PartialFileNameMatch, VarMatch, CmtMatch, MarkedMatch, DupFileNameMatch, UniqFileNameMatch, FirstFileNameMatch, LastFileNameMatch, TimeMatch, SizeMatch, ListIdxMatch }; /* FIXME */
	enum FilterAction{ Mark, Unmark, Delete }; /* FIXME */
	flist_t flist_; /* the names of files in the slideshow.  */
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 1
	flist_t hlist_;
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
#if FIM_EXPERIMENTAL_SHADOW_DIRS == 2
	std::vector<std::string> shadow_dirs_;
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
#if FIM_WANT_PIC_LBFL
	flist_t tlist_; /* the names of files in the slideshow.  */
	flist_t llist_; /* limited file list (FIXME: still unused) */
	fim_bool_t limited_; // FIXME: this->limited_
#endif /* FIM_WANT_PIC_LBFL */

	const fim_fn_t nofile_; /* a dummy empty filename */
	CommandConsole& commandConsole_;

#ifdef FIM_READ_STDIN_IMAGE
#if FIM_IMG_NAKED_PTRS
	ImagePtr default_image_;	// experimental
#else /* FIM_IMG_NAKED_PTRS */
	std::shared_ptr<Image> default_image_;	// experimental
#endif /* FIM_IMG_NAKED_PTRS */
#endif /* FIM_READ_STDIN_IMAGE */
	Viewport* viewport(void)const;

	int current_n(int ccp)const;
	const fim::string pop(fim::string filename=FIM_CNS_EMPTY_STRING, bool advance=false);
	
	fim_int current_image(void)const;
	public:
	flist_t get_file_list(void)const { return flist_; }
	int current_n(void)const;
	fim_int current_p(void)const;
	fim::string get_next_filename(int n)const; // FIXME: should be private
	fim::string last_regexp_; // was private
	fim_int last_src_dir_;
	Cache cache_;	// was private
#if FIM_WANT_BACKGROUND_LOAD
	PACA pcache_;	// was private
#endif /* FIM_WANT_BACKGROUND_LOAD */
#ifdef FIM_READ_STDIN_IMAGE
	void set_default_image(ImagePtr stdin_image);
#endif /* FIM_READ_STDIN_IMAGE */
	const Image* c_getImage(void)const;	// was private
	int empty_file_list(void)const;

	Browser(CommandConsole& cc);
	~Browser(void) FIM_OVERRIDE { }
#if FIM_WANT_PIC_CMTS_RELOAD
	std::vector<std::pair<string,fim_char_t> > dfl_; // FIXME: should be private
#endif /* FIM_WANT_PIC_CMTS_RELOAD */
	public:
	/* a deleted copy constructor (e.g. not even a be'friend'ed class can call it) */
	Browser& operator= (const Browser& rhs) = delete;
	Browser(const Browser& rhs) = delete;
	public:
	fim_fn_t current(void)const;
	fim::string regexp_goto(const args_t& args, fim_int src_dir=1);
	fim_cxr fcmd_prefetch(const args_t& args);
	fim_cxr fcmd_goto(const args_t& args);
	fim_goto_spec_t goto_image_compute(const fim_char_t *s, fim_xflags_t xflags)const;
	fim::string goto_image_internal(const fim_char_t *s, fim_xflags_t xflags);
	fim::string goto_image(fim_int n, bool isfg=false);
	fim_cxr fcmd_align(const args_t& args);
	fim::string fcmd_pan(const args_t& args);
	fim_cxr scrolldown(const args_t& args);
	fim_cxr scrollforward(const args_t& args);
	fim_cxr fcmd_scroll(const args_t& args);
	fim_cxr fcmd_scale(const args_t& args);
#if FIM_WANT_FAT_BROWSER
	//fim_cxr fcmd_no_image(const args_t& args);
#endif /* FIM_WANT_FAT_BROWSER */
	fim_cxr fcmd_rotate(const args_t& args);
	fim::string display_status(const fim_char_t *l);
	fim_cxr fcmd_color(const args_t& args);
	fim_cxr fcmd_crop(const args_t& args);

#if FIM_WANT_PIC_LBFL
	fim_cxr limit_to_variables(size_t min_vals, bool expanded)const;
	fim_cxr fcmd_limit(const args_t& args);
#endif /* FIM_WANT_PIC_LBFL */
	fim_cxr fcmd_reload(const args_t& args);
	fim_cxr fcmd_list(const args_t& args);
	fim::string do_push(const args_t& args);
	fim::string prev(int n=1);
	fim_cxr fcmd_info(const args_t& args);
#if FIM_WANT_OBSOLETE
	fim::string info(void);
#endif /* FIM_WANT_OBSOLETE */
	std::ostream& print(std::ostream& os)const;
	fim_cxr fcmd_load(const args_t& args);
	const fim::string pop_current(void);
	fim::string pop_current(const args_t& args);
	bool present(const fim::string nf)const;
	fim_int find_file_index(const fim::string nf)const;
#ifdef FIM_READ_DIRS
	bool push_dir(const fim::string & nf, fim_flags_t pf=FIM_FLAG_PUSH_REC, const fim_int * show_must_go_on=FIM_NULL);
#endif /* FIM_READ_DIRS */
	bool push_path(const fim::string & nf, fim_flags_t pf=FIM_FLAG_PUSH_REC, const fim_int * show_must_go_on=FIM_NULL);
	bool push_noglob(const fim::string & nf, fim_flags_t pf=FIM_FLAG_PUSH_REC, const fim_int * show_must_go_on=FIM_NULL);
#if FIM_EXPERIMENTAL_SHADOW_DIRS
	void push_shadow_dir(const std::string fn);
	fim_cxr shadow_file_swap(const fim_fn_t fn);
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */

	fim::string _random_shuffle(bool dts=true);
	fim::string _sort(const fim_char_t sc=FIM_SYM_SORT_FN, const char*id="");
	void _unique(void);
	fim::string _clear_list(void);
	private:
	fim::string do_filter_cmd(const args_t args, bool negative, enum FilterAction faction);
	fim::string do_filter(const args_t& args, MatchMode rm=FullFileNameMatch, bool negative=false, enum FilterAction faction = Delete);
	fim_err_t loadCurrentImage(void);
	fim::string reload(void);
	public:
	fim_int n_files(void)const;
	fim_int n_pages(void)const;
	private:
	fim::string next(int n=1);

	void free_current_image(bool force);
	int load_error_handle(fim::string c);
	public:
	fim::string _reverse(void);
	fim::string _swap(void);
	virtual size_t byte_size(void)const FIM_OVERRIDE;
	fim_float_t file_progress(void)const { /* FIXME: relies on range 0... */ double fp = (((double)(1+this->current_n()))/this->n_files()); return FIM_MIN(1.0,fp);} 
	void mark_from_list(const args_t& argsc);
	bool dump_desc(const fim_fn_t nf, fim_char_t sc, const bool want_all_vars=false, const bool want_append=false)const;
#if FIM_WANT_BENCHMARKS
	virtual fim_int get_n_qbenchmarks(void)const FIM_OVERRIDE;
	virtual string get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const FIM_OVERRIDE;
	virtual void quickbench_init(fim_int qbi) FIM_OVERRIDE;
	virtual void quickbench_finalize(fim_int qbi) FIM_OVERRIDE;
	virtual void quickbench(fim_int qbi) FIM_OVERRIDE;
#endif /* FIM_WANT_BENCHMARKS */
	bool filechanged(void);
}; /* Browser */
} /* namespace fim */
#endif /* FIM_BROWSER_H */
