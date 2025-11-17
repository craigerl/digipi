# $LastChangedDate: 2024-05-01 01:20:20 +0200 (Wed, 01 May 2024) $
#
HTMLDOCS=
if WITH_PERL
HTMLDOCS+=FIM.html
endif
if WITH_GROFF
HTMLDOCS+=fim.man.html fimgs.man.html fimrc.man.html
else
if WITH_MAN2HTML
HTMLDOCS+=fim.man.html fimgs.man.html fimrc.man.html
else
if WITH_MAN
HTMLDOCS+=fim.man.html fimgs.man.html fimrc.man.html
endif
endif
endif

CLEANFILES= doctags$(EXEEXT) *.o  tags $(HTMLDOCS)

EXTRA_DIST = FIM.TXT fim.man doctags.c fim-stylesheet.css ./vim2html.pl fimgs.man fimrc.man fim.man.in fimgs.man.in fimrc.man.in
if WITH_GROFF
MAN2HTML=groff -Thtml -man
else
if WITH_MAN2HTML
MAN2HTML=man2html
else
if WITH_MAN
MAN2HTML=$(MAN) --html=cat
endif
endif
endif
MAN2HTMLS = sed 's/&minus;/-/g'
MAN2TXT=$(MAN) --ascii # when having e.g. a minimal groff installation

man1_MANS = fim.man fimgs.man
man5_MANS = fimrc.man

# html stuff is installed in the documentation directories, too
htmldir = $(docdir)
dist_html_DATA = $(HTMLDOCS)
dist_doc_DATA = FIM.TXT

.PHONY: all
all:	$(HTMLDOCS) $(EXTRA_DIST)

FIM.html fim-stylesheet.css: doctags$(EXEEXT) FIM.TXT
	./doctags$(EXEEXT) $(srcdir)/FIM.TXT > tags ; $(srcdir)/vim2html.pl tags $(srcdir)/FIM.TXT

fim.man:
	make doc/fim.man -C $(abs_top_builddir)/

fim.man.html: fim.man
	$(MAN2HTML) ./$< | $(MAN2HTMLS) > $@
	if test ! -s $@ ; then $(MAN2TXT) ./$< > $<.txt && $(srcdir)/vim2html.pl tags $<.txt && mv $<.txt.html $@ ; fi

fimrc.man:
	make doc/fimrc.man -C $(abs_top_builddir)/

fimrc.man.html: fimrc.man
	$(MAN2HTML) ./$< | $(MAN2HTMLS) > $@
	if test ! -s $@ ; then $(MAN2TXT) ./$< > $<.txt && $(srcdir)/vim2html.pl tags $<.txt && mv $<.txt.html $@ ; fi

fimgs.man:
	make doc/fimgs.man -C $(abs_top_builddir)/

fimgs.man.html: fimgs.man
	$(MAN2HTML) ./$< | $(MAN2HTMLS) > $@
	if test ! -s $@ ; then $(MAN2TXT) ./$< > $<.txt && $(srcdir)/vim2html.pl tags $<.txt && mv $<.txt.html $@ ; fi

doctags$(EXEEXT): doctags.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

#clean:
#	        $(RM) -f  doctags.o doctags$(EXEEXT) FIM.html tags fim-stylesheet.css fim.man.html fimgs.man.html

