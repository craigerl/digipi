
# $Id: Makefile.am 1586 2018-05-28 22:57:26Z dezperado $

#
# The local Manifest building code is not enough
#

#Manifest: fim-0.1.ebuild Makefile
#	echo -en 'MD5 ' > Manifest
#	echo `md5sum fim-0.1.ebuild` `wc -c  fim-0.1.ebuild | sed 's/ .*//g'`  >> Manifest

subdir = distros


AUTOMAKE_OPTIONS = nostdinc no-dependencies
SUBDIRS = 


PREFIX=gentoo/media-gfx/fim

# 20090217 : who cares, anyway ?
# EXTRA_DIST=debian/control $(PREFIX)/fim-$(VERSION).ebuild

$(PREFIX)/Manifest: $(PREFIX)/fim-$(VERSION).ebuild Makefile
	cd $(PREFIX)/ && ebuild fim-$(VERSION).ebuild digest

.PHONY: all
all:

# well, this is of interest only of developer's interes
#all:	ebuild

.PHONY: ebuild
ebuild:	$(PREFIX)/Manifest

.PHONY: clean
clean:
	rm -fR $(PREFIX)/Manifest $(PREFIX)/files 

