Welcome to Tampa PingPong (TPP)!!

This is the Tampa Ping-Pong conversd source, derived from PingPong conversd.

Read the other README.* files, since TPP is a superset of this information.
This file will only deal with info specific to TPP.

Newer releases of this software might be found in the anonymous ftp directory
at lantz.com in the '/tpp/current' directory.

----------------------------------------------------------------------------

There is a mailing list for TPP support and questions, tpp-topics@lantz.com.
To join this mailing list, send a mail message to 'listserv@lantz.com' and in
the data of the message include a single line

	SUBSCRIBE emailaddress tpp-topics

where "emailaddress" is an optional name of a complete email address that
you wish the subscription sent. The "emailaddress" is only necessary if you
wish the subscription to go to an address other than the one you are sending
the email subscription request from. Example:

	SUBSCRIBE tpp-stuff@ko4ks.ampr.org tpp-topics

----------------------------------------------------------------------------

The convers.conf file format has changed COMPLETELY, so read this next
section carefully. Relax, it's really easier than before, but it IS different.

Here is the 'typical' convers.conf file for an ampr-net host, modified from
the example in the README file:

	Server		dc6iq-u
	Email		fred@db0sao.ampr.org
	Secretnum	12345
	Mode		amprnet
	# This is just a comment
	Link		dc1ik-u 1 1 dc1ik-u:3600
	Link		db0sao 2 1 unix:/tcp/.sockets/netcmd connect tcp db0sao 3600
	Link		db0me 3 1 unix:/tcp/.sockets/netcmd connect ax25 db0me
	Link		dc6iq-p 4 1 mailgate fred@behind-firewall.company.com


The data in the 'Link' lines is the same in the same order as before, just
preceeded with the 'Link' keyword.

The 'Server' line defines the conversd server name.

The 'Secretnum' line defines the secret number for operator priviledges.

The 'Email' line defines the email address of the primary sysop of this
conversd server, and is displayed as a part of the /sysinfo command.

The 'Mode' line defines the mode of the server, either 'amprnet' or 'internet'.

In addition to these lines, there are two other lines. The 'Security' line
defines the security level of the server. This is either 'restricted', which
only allows users defined in the convers.access file to login to the server,
or 'observer', which allows users not defined in the convers.access file to
login, but places them in OBSERVER state, or 'none' (the default), which
uses NO security access control.

The 'Sysinfo' line defines the additional system information given in
response to a local or remote /sysinfo command.

There is one additional parameter for the 'Link' line, which is used for
setting up backup links. Since this was Warren's 'contribution', here's the
original description from him, with the convers.conf format changed:

------------------------------------------------------------------------

Server		Myserver
Secretnum	123456
Link		Hydra 1 1 hydra.carleton.ca:3600
Link		Fred  1 1 fred:3600 Hydra


Fred is the backup link for Hydra. If you can't connect to Hydra, then the
link to Fred will occur. If/when Hydra comes back, the Fred link will
go down.

You can only have one backup, i.e a second line

Jim  1 1 jim:3600 Hydra

will make Jim the backup link to Hydra, and Fred will be a normal link.
However, you can _chain_ things, for example, if you do

Server		Myserver
Secretnum	123456
Link		Hydra 1 1 hydra.carleton.ca:3600
Link		Fred  1 1 fred:3600 Hydra
Link		Jim   1 1 jim:3600  Fred

then you get a Hydra <-> Fred <-> Jim backup chain.
The new bits are smart enough to walk up & down the chain when links go
up or down.

        Warren vk1xwt

-------------------------------------------------------------------

Many bugs and potential bugs were fixed from PingPong, as well as added
many features (most from TNOS's Conference Bridge), including:

*	protection against overrunning name/personal/topic buffers with strings
	that exceeded the buffer size

*	fixed a bug that occurred when entering the program with a non-default
	channel number that was greater than the highest channel number
	currently in use. This would wipe out all channel info.

*	finished mods to make all listings sorted in channel order.

*	fixed bug that prevented an operator from changing/removing a topic
	from a channel, unless he was ON the channel. This was a missing set of
	parenthesis in the source code.

*	added Warren's Backup Link patches, which will remain until the
	priority and groups are complete or replaced.

*	added an '/observer' command to the server (-o option to convers), that
	permits users to VIEW the Conference activity, but restricts them from
	ALL commands that produce output to others. They also cannot enter any
	text to others.

	Observers are NOT listed in non-TPP programs, since it send a new host
	command "/..OBSV", which serves the same purpose as the "/..USER"
	command, but identifies the 'user' as an observer-only. All non-TPP
	programs will ignore this command as unknown ;-)

*	added a '/restricted' command to the server (-r option to convers),
	that permits only users listed in a convers.access file to be able to
	access the server.

*	added an optional parameter to the convers.conf file in the first line
	which can be 'restrict', or 'observe'. If not set, the restrictmode is
	as it was before, i.e. non-restricted. If this parameter is set, then
	the '/name' command will be treated as the '/restricted' or the
	'/observer' command, if the user is not found in the convers.access
	file.

*	added code to forward on to all other connected hosts any 'host'-type
	commands received that it doesn't understand. Previously, any 'new'
	commands got bit-bucketted at a host that didn't recognize it, whether
	the next one down the line knew what to do with it or not. This will
	allow servers to add new functionality, without requiring all servers
	to add this in order for it to be used.

*	added a command ('/CQ  [topic]') by which a user could send a message
	to all users, which displays something like:

		callsign@host channel xx calling CQ [topic]

	for people to establish QSO's without having to go to any particular
	channel to see who's there and what's happening. It allows those who
	are looking for conversation partners to make it known!

*	changed the nickname code to output in a format of <call:nickname>
	if a user has a nickname defined.

*	added a command ('/sysinfo [host]') by which a user can get specific
	information about a Conference host.

*	added a way to allow the command set of a TPP system to be extended
	by external commands, provided by special servers.

*	the default display for a '/who' command used to be the long display.
	It is now the same as the quick display, making '/who' and '/who q'
	the same. To get the original, normal display, use '/who n'.

*	The syntax of '/who @ hostname' is now supported, which gives a normal
	(long) who display, but only of users at 'hostname'. The 'hostname'
	parameter can be a complete name, or only the beginning of a name,
	for brevity.

*	Added two operator mode commands, /restart and /halt.

	There was a suggestion by Heikki Hannikainen &<oh7lzb@zone.pspt.fix>
	for a new operator mode command to TPP, that I have added but made
	into TWO commands. Both of these require the local user to be an
	operator.

	The /restart command will disconnect all links and users, but NOT exit
	the server. This can be used to flush out a system, and help clear up
	any 'strange' users lingering around from links that no longer exist.

	The /halt command does all of the same things as the /restart command,
	and then goes into an infinite, do nothing loop, preventing links and
	users from re-connecting. Heikki had brought up a good point that
	several PP sysops have the PP set up so that when it exits, it comes
	back up again. Well, while this is good, it is not so good if there
	is a problem, like the long topics where causing a little while ago.
	This allows you to stop the server in its tracks, without requiring
	you to telnet into the box and transmit the Linux root password over
	the air.

	While the usefulness of these is admittedly limited, when you need
	them, you will NEED them ;-)
 
