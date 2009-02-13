# $Id: mk-0th.awk,v 1.17 2005/01/22 16:31:40 tom Exp $
##############################################################################
# Copyright (c) 1998-2004,2005 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1996,1997
#
# Generate list of sources for a library, together with lint/lintlib rules
#
# Variables:
#	libname (library name, e.g., "ncurses", "panel", "forms", "menus")
#	subsets (is used here to decide if wide-character code is used)
#
BEGIN	{
		using = 0;
		found = 0;
	}
	!/^[@#]/ {
		if (using == 0)
		{
			print  ""
			print  "# generated by mk-0th.awk"
			printf "#   libname:    %s\n", libname
			printf "#   subsets:    %s\n", subsets
			print  ""
			print  ".SUFFIXES: .c .cc .h .i .ii"
			print  ".c.i :"
			printf "\t$(CPP) $(CPPFLAGS) $< >$@\n"
			print  ".cc.ii :"
			printf "\t$(CPP) $(CPPFLAGS) $< >$@\n"
			print  ".h.i :"
			printf "\t$(CPP) $(CPPFLAGS) $< >$@\n"
			print  ""
			using = 1;
		}
		if ( $0 != "" && $1 != "link_test" )
		{
			if ( found == 0 )
			{
				if ( subsets ~ /widechar/ )
					widechar = 1;
				else
					widechar = 0;
				printf "C_SRC ="
				if ( $2 == "lib" )
					found = 1
				else
					found = 2
			}
			if ( libname == "c++" || libname == "c++w" ) {
				printf " \\\n\t%s/%s.cc", $3, $1
			} else if ( widechar == 1 || $3 != "$(wide)" ) {
				printf " \\\n\t%s/%s.c", $3, $1
			}
		}
	}
END	{
		print  ""
		if ( found == 1 )
		{
			print  ""
			printf "# Producing llib-l%s is time-consuming, so there's no direct-dependency for\n", libname
			print  "# it in the lintlib rule.  We'll only remove in the cleanest setup."
			print  "clean ::"
			printf "\trm -f llib-l%s.*\n", libname
			print  ""
			print  "realclean ::"
			printf "\trm -f llib-l%s\n", libname
			print  ""
			printf "llib-l%s : $(C_SRC)\n", libname
			printf "\tcproto -a -l -DNCURSES_ENABLE_STDBOOL_H=0 -DLINT $(CPPFLAGS) $(C_SRC) >$@\n"
			print  ""
			print  "lintlib :"
			printf "\tsh $(srcdir)/../misc/makellib %s $(CPPFLAGS)", libname
			print ""
			print "lint :"
			print "\t$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(C_SRC) $(LINT_LIBS)"
		}
		else
		{
			print  ""
			print  "lintlib :"
			print  "\t@echo no action needed"
		}
	}
