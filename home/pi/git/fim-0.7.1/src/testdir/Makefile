#
# Makefile to run all tests for Fim (Vim-styled!)
#

BASE=../../
FIMPROG = $(BASE)/src/fim

# Uncomment this line for using valgrind.
# The output goes into a file "valgrind.$PID" (sorry, no test number).
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=15 --log-file=valgrind.log

SCRIPTS = test1.out test2.out test3.out test4.out test5.out test6.out test7.out test8.out test9.out test10.out test11.out test12.out test13.out test14.out test15.out test16.out test17.out test18.out test19.out test20.out test21.out test22.out

.SUFFIXES: .in .out

nongui: nolog $(SCRIPTS)
	@echo
	@cat test.log
	@! grep FAILED test.log
	@echo "[*] ALL DONE"


$(SCRIPTS): $(FIMPROG)

clean:
	-rm -rf *.out *.failed *.rej *.orig test.log tiny.fim small.fim mbyte.fim test.ok X* valgrind.pid* fiminfo

.in.out:
	-rm -rf $*.failed test.ok test.out X* fiminfo
	cp $*.ok test.ok
	# Sleep a moment to avoid that the xterm title is messed up
	@-sleep .2
	-$(VALGRIND) $(FIMPROG) -o dumb                                           -c "quit;" -X -N --no-etc-fimrc-file --no-history -E $*.in $(BASE)/media/*jpg $(BASE)/media/fim.png $(BASE)/media/image.png > test.out
#	-$(VALGRIND) $(FIMPROG) -u unix.fim -U NONE --noplugin -s dotest.in $*.in
	@/bin/sh -c "if test -f test.out; then\
                  if diff $*.ok test.out; \
                  then mv -f test.out $*.out; \
                  else echo $* FAILED >>test.log; head -n 1 $*.in >>test.log; mv -f test.out $*.failed; \
                  fi \
                else echo $* NO OUTPUT >>test.log; \
                fi"
	-rm -rf X* test.ok fiminfo

nolog:
	-echo Test results: >test.log

