/* $LastChangedDate: 2024-03-31 12:29:10 +0200 (Sun, 31 Mar 2024) $ */
/*
 CommandConsole.h : Fim console dispatcher

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

#ifndef FIM_COMMANDCONSOLE_H
#define FIM_COMMANDCONSOLE_H
#include "fim.h"
#include "DummyDisplayDevice.h"

#define FIM_WANT_RAW_KEYS_BINDING 1
#define FIM_INVALID_IDX -1

namespace fim
{
class CommandConsole FIM_FINAL : 
#if FIM_WANT_BENCHMARKS
	public Benchmarkable,
#endif /* FIM_WANT_BENCHMARKS */
	public Namespace
{
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
#ifndef FIM_KEEP_BROKEN_CONSOLE
	public:
	MiniConsole mc_;
#endif /* FIM_KEEP_BROKEN_CONSOLE */
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	private:
	FontServer fontserver_;

	fim_cls postInitCommand_;  // Executed after preConfigCommand_.
	fim_cls preConfigCommand_; // Executed before postInitCommand_.
	fim_cls postExecutionCommand_; // Executed right before a normal termination of Fim.

	fim_int show_must_go_on_;
	fim_int return_code_;	/* new, to support the 'return' command */
	bool mangle_tcattr_;

#if HAVE_TERMIOS_H
	struct termios  saved_attributes_;
#endif /* HAVE_TERMIOS_H */
	fim_sys_int             saved_fl_; /* file status flags for stdin */

	public:
	Browser browser_;	/* the image browser_ logic */
	private:

#ifdef FIM_WINDOWS
	fim::FimWindow * window_;
#endif /* FIM_WINDOWS */
	/*
	 * the registered command member functions and objects
	 */
	std::vector<Command> commands_;			//command->member function

	/*
	 * the aliases to actions (compounds of commands)
	 */
	typedef std::map<fim::string,std::pair<fim_cmd_id,fim::string> > aliases_t;	//alias->[commands,description]
	//typedef std::map<fim::string,fim_cmd_id> aliases_t;	//alias->commands
	aliases_t aliases_;	//alias->commands
	
	/*
	 * bindings of key codes to actions (compounds of commands)
	 */
	public:
	typedef std::map<fim_key_t,fim_cmd_id> bindings_t;		//code->commands
	private:
	bindings_t bindings_;		//code->commands
	typedef std::map<fim_key_t,fim::string> bindings_help_t; // code->help
	bindings_help_t bindings_help_;		//code->commands

	/*
	 * mapping of key name to key code
	 */
	sym_keys_t	sym_keys_;	//symbol->code

	public:
	typedef std::map<fim_key_t, fim::string> key_syms_t;//code->symbol
	private:
	key_syms_t key_syms_;//code->symbol

	private:

	fim_err_t load_or_save_history(bool load_or_save);

#if FIM_WANT_FILENAME_MARK_AND_DUMP
	typedef std::set<fim_fn_t> marked_files_t;
	marked_files_t marked_files_;		//filenames
	public:
	bool isMarkedFile(fim_fn_t fname)const;
	fim::string marked_files_list(void)const;
	fim::string marked_files_clear(void);
	private:
#endif /* FIM_WANT_FILENAME_MARK_AND_DUMP */

	/*
	 * flags
	 */
#ifdef FIM_USE_READLINE
	/* no readline ? no console ! */
	fim_status_t 	ic_;				//in console if 1. not if 0. willing to exit from console mode if -1
#endif /* FIM_USE_READLINE */
	fim_cycles_t cycles_;					//fim execution cycles_ counter (quite useless)
	fim_key_t exitBinding_;				//The key bound to exit. If 0, the special "Any" key.

#ifdef FIM_AUTOCMDS
	/*
	 * the mapping structure for autocommands (<event,pattern,action>)
	 */
	typedef std::map<fim::string,args_t >  autocmds_p_t;	//pattern - commands
	typedef std::map<fim::string,autocmds_p_t >  autocmds_t;		//autocommand - pattern - commands
	autocmds_t autocmds_;
#endif /* FIM_AUTOCMDS */
	
	/*
	 * the last executed action (being a command line or key bounded command issued)
	 */
	fim_cls last_action_;
	
#ifdef FIM_ITERATED_COMMANDS
	fim_int it_buf_{-1};
#endif /* FIM_ITERATED_COMMANDS */
#ifdef FIM_RECORDING
	enum RecordMode { Recording, Playing, Normal };
	RecordMode recordMode_;
	typedef std::pair<fim::string,fim_tms_t > recorded_action_t;
	typedef std::vector<recorded_action_t > recorded_actions_t;
	recorded_actions_t recorded_actions_;

	bool dont_record_last_action_;
	fim::string memorize_last(const fim_cmd_id& cmd);
	fim::string repeat_last(const args_t& args);
	fim::string dump_record_buffer(const args_t& args);
	fim::string do_dump_record_buffer(const args_t& args, bool headeronly=false)const;
	fim::string execute_record_buffer(const args_t& args);
	fim::string start_recording(void);
	fim_cxr fcmd_recording(const args_t& args);
#if FIM_EXPERIMENTAL_FONT_CMD
	fim_cxr fcmd_font(const args_t& args);
#endif /* FIM_EXPERIMENTAL_FONT_CMD */
	fim::string stop_recording(void);
	fim::string sanitize_action(const fim_cmd_id& cmd)const;

	void record_action(const fim_cmd_id& cmd);
#endif /* FIM_RECORDING */

	public:
	fim_str_t fim_stdin_;	// the standard input file descriptor
	private:
	fim_char_t prompt_[2];

#ifndef FIM_WANT_NOSCRIPTING
	args_t scripts_;		//scripts to execute : FIX ME PRIVATE
#endif /* FIM_WANT_NOSCRIPTING */

#if FIM_WANT_FILENAME_MARK_AND_DUMP
	public:
	void markCurrentFile(bool mark = true);
	fim_int markFile(const fim_fn_t& file, bool mark = true, bool aloud = true);
	private:
#endif /* FIM_WANT_FILENAME_MARK_AND_DUMP */
#ifdef FIM_WITH_AALIB
	AADevice * aad_;
#endif /* FIM_WITH_AALIB */
	DummyDisplayDevice dummydisplaydevice_;

	DisplayDevice *displaydevice_;
	fim::string oldcwd_;
	public:
	fim_sys_int get_displaydevice_input(fim_key_t * c, bool want_poll=false);

	fim::string execute(fim_cmd_id cmd, args_t args, bool as_interactive=false, bool save_as_last=false, bool only_queue=false);

	//const fim_char_t*get_prompt(void)const{return prompt_;}

	CommandConsole(void);
	public:
	/* a deleted copy constructor (e.g. not even a be'friend'ed class can call it) */
	CommandConsole& operator= (const CommandConsole&cc) = delete;

	public:
	bool redisplay(void);
	fim_char_t * command_generator (const fim_char_t *text,int state,int mask)const;
	fim_perr_t execution(void);
	void executionCycle(void);
	fim_err_t init(fim::string device);
	fim_bool_t inConsole(void)const;
	~CommandConsole(void)FIM_OVERRIDE;

	/* the following group is defined in Namespace.cpp */
	fim_float_t getFloatVariable(const fim_var_id& varname)const;
	fim::string getStringVariable(const fim_var_id& varname)const;
	fim_int  getIntVariable(const fim_var_id& varname)const;
	const Var  getVariable(const fim_var_id& varname)const;
	fim_int  setVariable(const fim_var_id& varname,fim_int value);
#if FIM_WANT_LONG_INT
	int  setVariable(const fim_var_id& varname,    int value);
	unsigned int setVariable(const fim_var_id& varname,    unsigned int value);
#endif /* FIM_WANT_LONG_INT */
	fim_float_t setVariable(const fim_var_id& varname, fim_float_t value);
	const fim_char_t*  setVariable(const fim_var_id& varname,const fim_char_t*value);
	const Var& setVariable(const fim_var_id varname,const Var&value);
	const fim::string& setVariable(const fim_var_id varname,const fim::string&value);
	fim_bool_t isSetVar(const fim_var_id& varname)const;
	Namespace * rns(const fim_var_id varname);
	const Namespace * c_rns(const fim_var_id varname)const;
	fim_var_id rnid(const fim_var_id& varname)const;

	fim_var_t getVariableType(const fim_var_id& varname)const;
	bool push(const char * nf, fim_flags_t pf=FIM_FLAG_DEFAULT);
#if FIM_WANT_BACKGROUND_LOAD
	bool background_push(void);
#endif /* FIM_WANT_BACKGROUND_LOAD */
	fim_err_t executeStdFileDescriptor(FILE *fd);
	fim::string readStdFileDescriptor(FILE* fd, int*rp=FIM_NULL);
#ifndef FIM_WANT_NOSCRIPTING
	bool push_scriptfile(const fim_fn_t ns);
	bool with_scriptfile(void)const;
	fim_cxr fcmd_executeFile(const args_t& args);
#endif /* FIM_WANT_NOSCRIPTING */
	fim_cxr get_help(const fim_cmd_id& item, const char dl, const fim_hflags_t flt=FIM_H_ALL)const;
	private:
	bool push(const fim_fn_t nf, fim_flags_t pf=FIM_FLAG_DEFAULT);
	fim_cxr fcmd_echo(const args_t& args);
	fim::string do_echo(const args_t& args)const;
	fim_cxr fcmd_help(const args_t& args);
	fim_cxr fcmd_quit(const args_t& args);
	fim_cxr fcmd__stderr(const args_t& args);
	fim_cxr fcmd__stdout(const args_t& args);
	fim_cxr fcmd_foo (const args_t& args);
	fim_cxr fcmd_status(const args_t& args);
	fim_err_t executeFile(const fim_char_t *s);
	fim_err_t execute_internal(const fim_char_t *ss, fim_xflags_t xflags);

	fim_err_t addCommand(Command c);
	//Command &findCommand(fim_cmd_id cmd)const;
	int findCommandIdx(fim_cmd_id cmd)const;
	fim_cxr fcmd_alias(const args_t& args);
	fim::string alias(const fim_cmd_id& a, const fim_cmd_id& c, const fim_cmd_id& d="");
	public:// 20240125
	fim::string aliasRecall(fim_cmd_id cmd)const;
	private:
	fim_cxr fcmd_system(const args_t& args);
	fim_cxr fcmd_cd(const args_t& args);
	fim_cxr fcmd_pwd(const args_t& args);
	fim_cxr fcmd_sys_popen(const args_t& args);
	fim_cxr fcmd_pread(const args_t& args);
	public:// 20110601
	fim_err_t fpush(FILE *tfd);
	private:
	fim_cxr fcmd_set_interactive_mode(const args_t& args);
	fim_cxr fcmd_set_in_console(const args_t& args);
#ifdef FIM_AUTOCMDS
	fim_cxr fcmd_autocmd(const args_t& args);
	fim::string autocmd_del(const fim::string event, const fim::string pattern, const fim::string action);
	fim_cxr fcmd_autocmd_del(const args_t& args);
	public:// 20110601
	fim::string autocmd_add(const fim::string& event,const fim::string& pat,const fim_cmd_id& cmd);
	private:
	fim::string autocmds_list(const fim::string event, const fim::string pattern)const;
#endif /* FIM_AUTOCMDS */
	typedef std::pair<fim_fn_t, fim::string> autocmds_loop_frame_t;
	typedef std::pair<autocmds_loop_frame_t,fim_fn_t> autocmds_frame_t;
	typedef std::vector<autocmds_loop_frame_t > autocmds_stack__t;
	typedef std::vector<autocmds_frame_t > autocmds_stack_t;
	//typedef std::set<autocmds_frame_t> autocmds_stack_t;
	autocmds_stack__t autocmds_loop_stack;
	autocmds_stack_t autocmds_stack;
	fim_cxr fcmd_bind(const args_t& args);
	fim_key_t kstr_to_key(const fim_char_t * kstr)const;
	public:
	int pop_chars_press(fim_key_t *cp);
#if FIM_WANT_CMD_QUEUE
	std::vector<std::pair<fim_cmd_id,args_t>> cmdq_; /* command queue */
	bool execute_queued(void);
#endif /* FIM_WANT_CMD_QUEUE */
#if FIM_WANT_CMDLINE_KEYPRESS
	void push_chars_press(const char *cp);
	void push_key_press(const char *c_str);
	private:
	std::queue<fim_key_t,std::list<fim_key_t> > clkpv_; /* command line key presses vector*/
#endif /* FIM_WANT_CMDLINE_KEYPRESS */
	fim::string getAliasesList(FimDocRefMode refmode=DefRefMode)const;
	fim::string dummy(const args_t& args);
	fim_cxr fcmd_variables_list(const args_t& args);
	fim_cxr fcmd_commands_list(const args_t& args);
	fim_cxr fcmd_set(const args_t& args);
	fim_cxr fcmd_unalias(const args_t& args);
#ifdef FIM_ITERATED_COMMANDS
	fim_int cap_iterations_counter(fim_int it_buf)const;
#endif /* FIM_ITERATED_COMMANDS */
	bool executeBinding(const fim_key_t c);
	fim::string getBoundAction(const fim_key_t c)const;
	//	void execute(fim_cmd_id cmd);
	fim_cxr fcmd_eval(const args_t& args);
	FIM_NORETURN void exit(fim_perr_t i)const;// FIXME: exit vs quit
	fim::string unbind(fim_key_t c);
	fim::string bind(const fim_key_t c, const fim_cls binding, const fim::string hstr="", const bool help_only=false);
	public:
	fim::string find_key_for_bound_cmd(fim_cls cmd)const;
	fim_err_t execDefaultConfiguration(void);
	private:
	fim::string unbind(const fim::string& key);
	fim_cxr fcmd_unbind(const args_t& args);
	fim::string getBindingsList(void)const;
	fim_cxr fcmd_dump_key_codes(const args_t& args);
	fim::string do_dump_key_codes(FimDocRefMode refmode=DefRefMode)const;
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	fim_cxr fcmd_clear(const args_t& args);
	fim::string scroll_up(const args_t& args);
	fim::string scroll_down(const args_t& args);
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	fim_perr_t quit(fim_perr_t i=FIM_CNS_ERR_QUIT);
	public:
	fim_key_t find_keycode_for_bound_cmd(fim_cls binding)const;

	fim_bool_t drawOutput(const fim_char_t*s=FIM_NULL)const;
	bool regexp_match(const fim_char_t*s, const fim_char_t*r, int rsic)const;
#ifdef FIM_AUTOCMDS
	fim::string autocmd_exec(const fim::string& event,const fim_fn_t& fname);
	fim::string pre_autocmd_add(const fim_cmd_id& cmd);
	fim::string pre_autocmd_exec(void);
#endif /* FIM_AUTOCMDS */
	fim_int catchLoopBreakingCommand(fim_ts_t seconds=0);

	private:
	/* fim_key_t catchInteractiveCommand(fim_ts_t seconds=0)const; */
#ifdef FIM_AUTOCMDS
	fim::string autocmd_exec(const fim::string& event,const fim::string& pat,const fim_fn_t& fname);
	void autocmd_push_stack(const autocmds_loop_frame_t& frame);
	void autocmd_pop_stack(const autocmds_loop_frame_t& frame);
	public:
	void autocmd_trace_stack(void);
	private:
	fim_bool_t autocmd_in_stack(const autocmds_loop_frame_t& frame)const;
#endif /* FIM_AUTOCMDS */
	fim::string current(void)const;

	fim::string get_alias_info(const fim::string aname, FimDocRefMode refmode=DefRefMode)const;
#ifdef FIM_WINDOWS
	const FimWindow& current_window(void)const;
#endif /* FIM_WINDOWS */
	public:
	fim::string get_variables_list(void)const;
	fim::string get_commands_list(void)const;
	fim::string get_aliases_list(void)const;
	bindings_t get_bindings(void)const;

	void printHelpMessage(const fim_char_t *pn="fim")const;
	void appendPostInitCommand(const fim::string& c);
	void appendPreConfigCommand(const fim::string& c);
	void appendPostExecutionCommand(const fim::string& c);
	bool appendedPostInitCommand(void)const;
	bool appendedPreConfigCommand(void)const;

	Viewport* current_viewport(void)const;
#ifdef FIM_WINDOWS
	/* Viewport is managed by FimWindow */
#else /* FIM_WINDOWS */
	Viewport* viewport_;
#endif /* FIM_WINDOWS */
	void dumpDefaultFimrc(void)const;

	void tty_raw(void);
	void tty_restore(void);
	void cleanup(void);
	
	fim::string print_commands(void)const;

	void status_screen(const fim_char_t *desc);
	bool set_status_bar(fim::string desc, const fim_char_t *info);
	bool set_status_bar(const fim_char_t *desc, const fim_char_t *info);
        bool is_file(fim::string nf)const;
	fim_cxr fcmd_do_getenv(const args_t& args);
	bool isVariable(const fim_var_id& varname)const;
	fim::string dump_reference_manual(const args_t& args);
	fim::string get_reference_manual(const args_t& args);
	private:
	fim::string get_commands_reference(FimDocRefMode refmode=DefRefMode)const;
	fim::string get_variables_reference(FimDocRefMode refmode=DefRefMode)const;
	public:
	bool set_wm_caption(const fim_char_t *str);
	fim_err_t display_resize(fim_coo_t w, fim_coo_t h, fim_bool_t wsl = false);
	fim_err_t display_reinit(const fim_char_t *rs);
	fim_cxr fcmd_basename(const args_t& args);
	fim_cxr fcmd_desc(const args_t& args);
	fim_cxr fcmd_display(const args_t& args);
	fim_cxr fcmd_redisplay(const args_t& args);
	fim_bool_t key_syms_update(void);
	fim::string find_key_sym(fim_key_t key, bool extended)const;
#if FIM_WANT_BENCHMARKS
	virtual fim_int get_n_qbenchmarks(void)const FIM_OVERRIDE;
	virtual string get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const FIM_OVERRIDE;
	virtual void quickbench_init(fim_int qbi) FIM_OVERRIDE;
	virtual void quickbench_finalize(fim_int qbi) FIM_OVERRIDE;
	virtual void quickbench(fim_int qbi) FIM_OVERRIDE;
#endif /* FIM_WANT_BENCHMARKS */
	virtual size_t byte_size(void)const FIM_OVERRIDE;
	public:
#if FIM_WANT_PIC_CMTS
	ImgDscs id_;
	bool push_from_id(void);
#endif /* FIM_WANT_PIC_CMTS */
#if FIM_WANT_BACKGROUND_LOAD
	private:
	std::thread blt_; /* background loader thread */
	std::vector<const char *> fnpv_; /* file names pointers vector */
#endif /* FIM_WANT_BACKGROUND_LOAD */
	fim_err_t update_font_size(void);
	void internal_status_changed();
	public:
	void switch_if_needed(void);
	fim::string getInfoCustom(const fim_char_t * ifsp)const;
	fim_int show_must_go_on(void) const;
};
}

#endif /* FIM_COMMANDCONSOLE_H */
