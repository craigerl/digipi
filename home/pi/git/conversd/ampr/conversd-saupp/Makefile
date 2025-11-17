#
# @(#) $Id: Makefile,v 1.4 2005/02/15 16:31:28 dl9sau Exp $
#
# Makefile for htpp conversd
#   by Heikki Hannikainen
#

#
# What to use for compiler/linker. Someone might prefer egcs.
CC = gcc

#
# Directories:
#
# This is what a local Linux FSSTND compliant installation would look like:
# /usr if you package it in the OS:
BASE_DIR = /usr/local
INIT_DIR = /etc/init.d
#DATA_DIR = /var/cache/conversd
DATA_DIR = /run/conversd
# /etc/conversd if you...
CONF_DIR = $(BASE_DIR)/etc/conversd
BIN_DIR = $(BASE_DIR)/bin
SBIN_DIR = $(BASE_DIR)/sbin
MAN_DIR = $(BASE_DIR)/man
#
#
# hostname the convers client will connect
HN = localhost

# Owners: (there is NO reason to run conversd as root, better the opposite!)
OWN = daemon
GRP = daemon
MAN_OWN = root 
# on MACOSX: wheel
#MAN_GRP = wheel
MAN_GRP = root

# on MACOSX: wheel
#BIN_GRP = wheel
BIN_GRP = root

# what do you prefer?
#ONLINE_PRG=online
ONLINE_PRG=online.sh

# convers client: define a default channel (default: conversd's default
# channel). edit the source of convers.c and look at DEFAULT_CHAN definitions

#
# Options:
#
#  BAN_ZERO	Forbid access to channel 0 from local users. Channel 0
#		traffic is still forwarded to links.
#		obsolete - now a config file option
#  WANT_LOG	Log user logins/logouts in var/log/ directory, for the
#		/last command.
#
#  NO_SNPRINTF	For compiling under pre-2.6 Solaris, Digital Unix and
#		others without snprintf() and vsnprintf() (they're
#		non-standard, mainly GNU extensions i think)
#  FORKPTY	if you have libutil and libutil-dev
#               Note: not available on MACOSX
#  
#COPTS = -DWANT_LOG
# on MACOSX us this:
#COPTS = -DWANT_LOG

COPTS = -DWANT_LOG -DFORKPTY

#
# Optimisation: -O2 for optimised slightly faster and smaller "production"
# code, -g if you want to run gdb on it or it's cores
#COPTIM = -g
COPTIM = -O2 -s

#
# Warnings: If compiling using something than gcc, must usually drop these    
# away.
CWARNS = -Wall -Wstrict-prototypes

#
# Extra libraries
#
# These two needed on Solaris:
#EXTRALIBS = -lsocket -lnsl
# on MACOSX us this:
#EXTRALIBS =
# if you have forkpty, use this:
#EXTRALIBS = -lutil
EXTRALIBS = -lutil

# comment the following two lines out if you don't have / want libreadline
# you do not have it on MACOSX
# if readline.h is in /usr/include/, use -DHAVE_READLINE_H,
# if readline.h is in /usr/include/readline, use -DHAVE_READLINE_READLINE_H
# craiger no readline
#READLINE_INCLUDES = -DHAVE_LIBREADLINE -DHAVE_READLINE_READLINE_H
#READLINE_LIBS = -lreadline -lncurses

#
# There should be no need to touch anything below this line.
CHOST = -DCONVERSHOST=\"$(HN)\"
CDIRS = -DCONF_DIR=\"$(CONF_DIR)\" -DDATA_DIR=\"$(DATA_DIR)\"
CFLAGS = $(COPTIM) $(CWARNS) $(COPTS) -DOWNER=\"$(OWN)\" -DGID=\"$(GRP)\" $(CDIRS) $(CHOST)
LIBS = $(EXTRALIBS)

EXECS = conversd rawconvers convers ax25 axconv online

all: $(EXECS)

install: all install-bin install-man

install-all:
	@echo "warning: your configuration will be overwritten with"
	@echo "         the provided example files."
	@echo "         do you really want to do this?"
	@echo "	       anyway, a backup is made for every file, as filename~"
	@echo "         press ^C to interrupt"
	sleep 10
	make install install-conf

install-man:
	install -d -m 755 -o $(MAN_OWN) -g $(MAN_GRP) $(MAN_DIR)/man8
	install -m 444 -o $(MAN_OWN) -g $(MAN_GRP) man/conversd.8 $(MAN_DIR)/man8
	install -m 444 -o $(MAN_OWN) -g $(MAN_GRP) man/convers.1 $(MAN_DIR)/man1
	install -m 444 -o $(MAN_OWN) -g $(MAN_GRP) man/ax25.1 $(MAN_DIR)/man1
	install -m 444 -o $(MAN_OWN) -g $(MAN_GRP) man/cstat.1 $(MAN_DIR)/man1
	install -m 444 -o $(MAN_OWN) -g $(MAN_GRP) man/online.1 $(MAN_DIR)/man1

install-bin: all
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/log
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/personals
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/nicknames
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/conversrc
	install -d -m 775 -o $(OWN) -g $(GRP)	$(DATA_DIR)/sockets
	# user's migrating? - well, he may have wrong permissions
	chown -R $(OWN).$(GRP) $(DATA_DIR)/
	install -d -m 755 -o root -g $(BIN_GRP)	$(SBIN_DIR)
	install -m 755 -o root -g $(BIN_GRP) 	conversd $(SBIN_DIR)/
	ln -f -s $(SBIN_DIR)/conversd $(SBIN_DIR)/lconversd
	ln -f -s $(SBIN_DIR)/conversd $(SBIN_DIR)/wconversd
	ln -f -s $(SBIN_DIR)/conversd $(SBIN_DIR)/ppconversd
	install -m 755 -o root -g $(BIN_GRP) 	ax25 $(BIN_DIR)/
	install -m 755 -o root -g $(BIN_GRP) 	axconv $(BIN_DIR)/
	install -m 755 -o root -g $(BIN_GRP) 	convers $(BIN_DIR)/
	install -m 755 -o root -g $(BIN_GRP) 	rawconvers $(BIN_DIR)/
	ln -f -s $(BIN_DIR)/convers $(BIN_DIR)/lconvers
	ln -f -s $(BIN_DIR)/convers $(BIN_DIR)/wconvers
	ln -f -s $(BIN_DIR)/convers $(BIN_DIR)/ppconvers
	install -m 755 -o root -g $(BIN_GRP) $(ONLINE_PRG) $(BIN_DIR)/online
	ln -f -s $(BIN_DIR)/online $(BIN_DIR)/cstat
	ln -f -s $(BIN_DIR)/online $(BIN_DIR)/map
	install -d -m 755 -o $(OWN) -g $(GRP)	$(CONF_DIR)
	install -m 644 -o $(OWN) -g $(GRP)	etc/convers.help $(CONF_DIR)/convers.help.en
	install -m 644 -o $(OWN) -g $(GRP)	etc/convers.help.de $(CONF_DIR)/convers.help.de

install-conf:
	install -d -m 755 -o $(OWN) -g $(GRP)	$(CONF_DIR)
	# user's migrating? - well, he may have wrong permissions
	chown -R $(OWN).$(GRP) $(CONF_DIR)/
	install -b -m 640 -o root -g $(BIN_GRP)		etc/conversd.conf $(CONF_DIR)/conversd.conf
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.motd $(CONF_DIR)/conversd.motd
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.issue $(CONF_DIR)/conversd.issue
	install -b -m 640 -o root -g $(BIN_GRP)		etc/conversd.conf $(CONF_DIR)/ppconversd.conf
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.motd $(CONF_DIR)/ppconversd.motd
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.issue $(CONF_DIR)/ppconversd.issue
	install -b -m 640 -o root -g $(BIN_GRP)		etc/conversd.conf $(CONF_DIR)/lconversd.conf
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.motd $(CONF_DIR)/lconversd.motd
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.issue $(CONF_DIR)/lconversd.issue
	install -b -m 640 -o root -g $(BIN_GRP)		etc/conversd.conf $(CONF_DIR)/wconversd.conf
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.motd $(CONF_DIR)/wconversd.motd
	install -b -m 644 -o $(OWN) -g $(GRP)	etc/convers.issue $(CONF_DIR)/wconversd.issue
	install -b -m 644 -o $(OWN) -g $(GRP)      etc/convers.news $(CONF_DIR)
	install -b -m 644 -o $(OWN) -g $(GRP)      etc/spam.txt $(CONF_DIR)
	install -b -m 644 -o $(OWN) -g $(GRP)      etc/smileys.txt $(CONF_DIR)

	install -b -m 755 -o root -g $(BIN_GRP)		etc/conversd.rc.d $(INIT_DIR)/conversd
	install -b -m 755 -o root -g $(BIN_GRP)		etc/ppconversd.rc.d $(INIT_DIR)/ppconversd
	install -b -m 755 -o root -g $(BIN_GRP)		etc/lconversd.rc.d $(INIT_DIR)/lconversd
	install -b -m 755 -o root -g $(BIN_GRP)		etc/wconversd.rc.d $(INIT_DIR)/wconversd
	ln -fs $(CONF_DIR)/convers.help.en $(CONF_DIR)/convers.help

clean:
	rm -rf *.o *~ */*~

distclean: clean
	rm -f $(EXECS)

.c.o:
	$(CC) $(CFLAGS) -c $<

CONVERSD_MODS = conversd.o user.o host.o irc.o convert.o config.o tnos.o log.o cfgfile.o access.o noflood.o hmalloc.o ba_stub.o compression.o md5.o md5_ampr.o 

conversd: $(CONVERSD_MODS)
	$(CC) $(CFLAGS) -o conversd $(CONVERSD_MODS) $(LIBS)

ax25: ba_stub.o convers.c
	$(CC) $(CFLAGS) -DAX25 \
		convers.c ba_stub.o -o ax25

rawconvers: ba_stub.o convers.c
	$(CC) $(CFLAGS) -DNO_SHELL -DRAW -DNO_IOCTL \
		convers.c ba_stub.o -o rawconvers

convers: ba_stub.o convers.c
	$(CC) $(CFLAGS) -DSETLOGNAMES $(READLINE_INCLUDES)   \
		convers.c ba_stub.o -o convers $(READLINE_LIBS)

online: online.o ba_stub.o
	$(CC) $(CFLAGS) -DCONVERSHOST=\"$(HN)\" online.o ba_stub.o  \
		-o online

axconv: axconv.o
	$(CC) $(CFLAGS) \
		axconv.c -o axconv

conversd.o: conversd.c conversd.h convert.h user.h irc.h host.h version.h tnos.h log.h cfgfile.h config.h access.h hmalloc.h ba_stub.h compression.h noflood.h
user.o: user.c user.h conversd.h irc.h host.h convert.h tnos.h version.h tnos.h log.h config.h hmalloc.h ba_stub.h noflood.h
irc.o: irc.c irc.h user.h conversd.h host.h convert.h tnos.h version.h tnos.h log.h config.h hmalloc.h ba_stub.h noflood.h
host.o: host.c host.h conversd.h user.h irc.h convert.h version.h log.h config.h hmalloc.h ba_stub.h noflood.h
tnos.o: tnos.c tnos.h conversd.h version.h user.h irc.h host.h log.h config.h noflood.h
config.o: config.c config.h cfgfile.h log.h access.h hmalloc.h
convert.o: convert.c convert.h hmalloc.h
log.o: log.c log.h config.h
cfgfile.o: cfgfile.c cfgfile.h hmalloc.h
access.o: access.c access.h log.h hmalloc.h config.h ba_stub.h md5_ampr.h conversd.h
noflood.o: noflood.c config.h conversd.h
md5_ampr.o: md5_ampr.c md5_ampr.h md5.h
hmalloc.o: hmalloc.c hmalloc.h log.h
compression.o: compression.c compression.h
ba_stub.o: ba_stub.c
convers.o: convers.c ba_stub.h
rawconvers.o: convers.c ba_stub.h
ax25.o: convers.c ba_stub.h
online.o: online.c ba_stub.h
axconv.o: axconv.c
