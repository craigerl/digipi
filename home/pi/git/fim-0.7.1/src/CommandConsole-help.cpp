/* $LastChangedDate: 2024-04-15 23:19:28 +0200 (Mon, 15 Apr 2024) $ */
/*
 CommandConsole-help.cpp : Fim console dispatcher--help member functions

 (c) 2011-2024 Michele Martone

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

namespace fim
{
	fim::string CommandConsole::get_variables_reference(FimDocRefMode refmode)const
	{
		/*
		 * returns the reference of registered functions
		 */
		fim::string s;
		s+= fim_get_variables_reference(refmode);
		// ...
		return s;
	}

	fim::string CommandConsole::get_commands_reference(FimDocRefMode refmode)const
	{
		/*
		 * returns the reference of registered commands.
		 */
		std::ostringstream oss;
		if(refmode==Man)
			goto manmode;
		for(size_t i=0;i<commands_.size();++i)
			oss << (commands_[i].cmd()) <<" : " <<(commands_[i]).getHelp() <<"\n";
		return oss.str();
manmode:
		for(size_t i=0;i<commands_.size();++i)
		{
			string chs =(commands_[i]).getHelp();
			chs.substitute(FIM_CNS_CMDSEP,"\n.fi\n");
			oss << ".na\n" /* No output-line adjusting; unelegant way to avoid man --html=cat's: cannot adjust line */
				<< ".B\n" << (commands_[i].cmd()) << "\n.fi\n"
				<< chs << "\n" << ".fi\n" << "\n";
		}
		fim::string s(oss.str());
		s.substitute("\\$","$\\:"); /* Zero-width break point on $ (that is, on long hardcoded regexps). */
		return s;
	}

	fim::string CommandConsole::print_commands(void)const
	{
		cout << "VARIABLES: "<<get_variables_list()<<"\n";
		cout << "COMMANDS: "<<get_commands_list()<<"\n";
		cout << "ALIASES: "<<get_aliases_list()<<"\n";
		return FIM_CNS_EMPTY_RESULT;
	}

	fim::string CommandConsole::get_reference_manual(const args_t& args)
	{
		/*
		 * dump textually a reference manual from all the available fim language features.
		 */
#include "grammar.h"
#include "examples.h"
#include "conf.h"
#include "help-acm.cpp"
		fim::string man_fimrc;
		FimDocRefMode refmode=DefRefMode;
		if(args.size()==1 && args[0]=="man")
		{
			refmode=Man;
			man_fimrc = 
			".\\\"\n"
			".\\\" $Id""$\n"
			".\\\"\n"
			".TH fimrc 5 \"(c) 2011-" FIM_CNS_LCY " " FIM_AUTHOR_NAME "\"\n"
		".SH NAME\n"
			"fimrc - \\fB fim \\fP configuration file and language reference\n"
			"\n"
		".SH SYNOPSIS\n"
			".B " FIM_CNS_USR_RC_COMPLETE_FILEPATH "\n.fi\n"
			".B " FIM_CNS_SYS_RC_FILEPATH "\n.fi\n"
			FIM_MAN_Bh("fim", "--" FIM_OSW_SCRIPT_FROM_STDIN " [ " FIM_CNS_EX_OPTIONS " ] < " FIM_CNS_EX_SCRIPTFILE "")
			FIM_MAN_Bh("fim", "--" FIM_OSW_EXECUTE_SCRIPT " " FIM_CNS_EX_SCRIPTFILE " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_EXECUTE_COMMANDS " " FIM_CNS_EX_CMDS_STRING " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_FINAL_COMMANDS " " FIM_CNS_EX_CMDS_STRING " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_DUMP_SCRIPTOUT " " FIM_CNS_EX_SCRIPTFILE " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_DUMP_SCRIPTOUT " " FIM_LINUX_STDOUT_FILE " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_CHARS_PRESS " " ":" FIM_CNS_EX_CMDS_STRING " [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_CHARS_PRESS " " ":" FIM_CNS_EX_CMDS_STRING " --" FIM_OSW_CHARS_PRESS " '' [ " FIM_CNS_EX_OPTIONS " ]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_KEYSYM_PRESS " " FIM_CNS_EX_KSY_STRING " [ " FIM_CNS_EX_OPTIONS " ]")
			"\n"
		".SH DESCRIPTION\n"
			"This page explains the \n.B fim\nscripting language, which is used for the \n.B fimrc\nconfiguration files, " FIM_CNS_EX_SCRIPTFILE "s, or " FIM_CNS_EX_CMDS_STRING " passed via command line " FIM_CNS_EX_OPTIONS ".\n"
			"This language can be used to issue commands (or programs) from the internal program command line accessed interactively by default through the \"" FIM_SYM_CONSOLE_KEY_STR "\" key (which can be customized via the \"" FIM_MAN_fB(FIM_VID_CONSOLE_KEY) "\" variable).\n"
			"One may exit from command line mode by pressing the " FIM_KBD_ENTER " key on an empty line (a non empty command line would be submitted for execution), or the " FIM_KBD_ESC " key "
#if FIM_WANT_DOUBLE_ESC_TO_ENTER
		       	" (in non-SDL mode, it is required to press the " FIM_KBD_ESC " key twice).\n"
#else /* FIM_WANT_DOUBLE_ESC_TO_ENTER */
		       	" (only in SDL mode).\n"
#endif /* FIM_WANT_DOUBLE_ESC_TO_ENTER */
			"The general form of a fim command/program is shown in the next section.\n"
#ifndef FIM_COMMAND_AUTOCOMPLETION
			"\nInterpretation of commands or aliases may use autocompletion (if enabled; see the " FIM_MAN_fB(FIM_VID_CMD_EXPANSION) " variable description), in a way to allow the user to type only the beginning of the command of interest.\n"
#endif /* FIM_COMMAND_AUTOCOMPLETION */
			"\n"
			"\n"
		".SH FIM LANGUAGE GRAMMAR\n"
			"This section specifies the grammar of the \n.B fim\nlanguage.\n\n"
			"Language elements surrounded by a single quote (\"'\") are literals.\n\n"
			"Warning: at the present state, this grammar has conflicts. A future release shall fix them.\n"
			"\n"
			+string(FIM_DEFAULT_GRAMMAR_FILE_CONTENTS)+
			"\n"
			"A STRING can be either a single quoted string or a double quoted string.\n"
			"A floating point number can be either unquoted (UNQUOTED_FLOAT) or quoted (QUOTED_FLOAT).\n"
			"A QUOTED_FLOAT is a floating point number, either single (\"'\") or double (\"\"\") quoted.\n"
			"An INTEGER shall be an unsigned integer number.\n"
			"An IDENTIFIER shall be one of the valid fim commands (see \n.B COMMANDS REFERENCE\n) or a valid alias.\n"
			"A VARIABLE shall be an already declared or undeclared variable identifier (see \n.B VARIABLES REFERENCE\n) or a valid alias, created using the \n.B alias\ncommand.\n"
			"The \"=~\" operator treats the right expression as a STRING, and uses it as a regular expression for matching purposes.\n"
			"The SLASH_AND_REGEXP is a slash (\"/\") followed by a STRING, interpreted as a regular expression.\n"
			"If 'INTEGER , INTEGER IDENTIFIER arguments' is encountered, command IDENTIFIER will be repeated on each file in the interval between the two INTEGERs, and substituting the given file name to any '{}' found in the commands arguments (which must be quoted in order to be treated as strings).\n"
			"See ""\\fR\\fI""regex""\\fR""(1) for regular expression syntax.\n"
			"\n"
			"The way some one-line statements are evaluated:\n\n"
			FIM_INTERNAL_LANGUAGE_SHORTCUT_SHORT_HELP
			"\n"
			// TODO: shall specify the identifier form
			// TODO: shall specify min and max ranges, signedness
			// TODO: place some working example here.
			// TODO: shall write about the conversion rules.
			"\n"
		".SH COMMANDS REFERENCE\n"
			"\n"
			+get_commands_reference(refmode)+
		".SH KEYSYMS REFERENCE\n"
			"\n"
			+do_dump_key_codes( Man )+
			"\n"
		".SH AUTOCOMMANDS REFERENCE\n"
			"Available autocommands are: "
			FIM_AUTOCOMMANDS_LIST
			" and they are triggered on actions as suggested by their name.\n"
			" Those associated to actual commands are mentioned in the associated commands reference.\n"
		".SH VARIABLES REFERENCE\n"
			"If undeclared, a variable will evaluate to 0.\n\n"
			"When assigning a variable to a string, use single or double quoting, otherwise it will be treated as a number.\n\n"
			"The namespaces in which variables may exist are: " FIM_SYM_NAMESPACE_PREFIXES_DSC ". A namespace is specified by a prefix, which can be: " FIM_SYM_NAMESPACE_PREFIXES ", be prepended to the variable name. The global namespace is equivalent to the empty one:''. The special variable " FIM_SYM_NAMESPACE_IMAGE_ALL_STR " expands to the collation of all the name-value pairs for the current image.\n"
			"\nIn the following, the [internal] variables are the ones referenced in the source code (not including the hardcoded configuration, which may be inspected and/or invalidated by the user at runtime).\n"
			"\n"
		       	+get_variables_reference(refmode)+
		".SH DEFAULT ALIASES REFERENCE\n"
			"Hardcoded aliases are: \n"
			"\n.fi\n"
		       	+getAliasesList(refmode)+
			"\n.fi\n"
			"They can be redefined with "
			"\n.B " FIM_FLT_ALIAS "\nor deleted with the "
			"\n.B " FIM_FLT_UNALIAS "\ncommand."
			"\n.fi\n"
			"Further default aliases are usually loaded at startup -- see the CONFIGURATION FILE EXAMPLE section below.\n"
		".SH COMMAND LINE USAGE EXAMPLES\n"
			".nf\n"
			+FIM_DEFAULT_EXAMPLE_FILE_CONTENTS+
			"\n"
		".SH CONFIGURATION FILE EXAMPLE\n"
			"Part of the default configuration comes from the " FIM_MAN_fB(FIM_VID_FIM_DEFAULT_CONFIG_FILE_CONTENTS) " variable, shown here."
			"\n.nf\n"
			"One can skip its loading by using the " FIM_MAN_fB(FIM_VID_NO_DEFAULT_CONFIGURATION) " variable.\n"
			"\n.nf\n"
			+fim_text_to_man(FIM_DEFAULT_CONFIG_FILE_CONTENTS)+
			"\n"
		".SH NOTES\n"
			"This manual page could be improved.\n"
			"Certain side effects of commands are not documented.\n"
			"Neither a formal description of the various commands.\n"
			"Interaction of commands and variables is also not completely documented.\n"
		".SH BUGS\n"
			"The\n.B fim\nlanguage shall be more liberal with quoting.\n"
		".SH SEE ALSO\n"
			"" FIM_MAN_fR("fim") "(1), " FIM_MAN_fR("fimgs") "(1), " FIM_MAN_fR("regex") "(1).\n"
		".SH AUTHOR\n"
			FIM_AUTHOR
			"\n"
		".SH COPYRIGHT\n"
			"See copyright notice in ""\\fR\\fI""fim""\\fR""(1).\n"
			"\n"
			"\n"
			;
		}
		else
			man_fimrc = 
				string("commands:\n")+
				get_commands_reference()+
				string("variables:\n")+
				get_variables_reference();
		return man_fimrc;
	}

	fim::string CommandConsole::dump_reference_manual(const args_t& args)
	{
		/*
		 * dump textually a reference manual from all the available fim language features.
		 */
		std::cout << get_reference_manual(args);
		return FIM_CNS_EMPTY_RESULT;
	}
} /* namespace fim */
