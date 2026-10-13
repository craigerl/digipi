#!/bin/bash 
# Version 0.2.4 PE1RRR WEB Portal for Packet
#
# Configuration
# 
LynxBin="/usr/bin/lynx"  # sudo apt install lynx
CurlBin="/usr/bin/curl"  # sudo apt install curl
UserAgent="User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:58.0) Gecko/20100101 Firefox/58.0"
WebLogFile="/tmp/bpq-browser.log" # sudo touch /var/log/bpq-browser; sudo chmod bpq:bpq /var/log/bpq-browser
Version="0.2.4"

# It is recommended to set up a proxy server locally to handle the requests from this script
# it adds a level of control over which content can and cannot be requested, squid proxy is
# utilized for this, but if you do not want to use it, comment the 2 lines out below.
# I have set up my squid proxy to use alternate DNS servers of OpenDNS FamilyShield.

#myproxy="http://127.0.0.1:3128"
#export http_proxy=$myproxy
# 
# Usage & Installation
#
# For simplicity, I use openbsd's inetd which is available for Debian/Raspbian with sudo apt install openbsd-inetd
# 
# Add the following line to /etc/inetd.conf
# 
# browse		stream	tcp	nowait	bpq		/full/path/to/your/packet-scriptlets/browse.sh client ax25
# 
# The word 'bpq' above refers to the userid that this process will run under.
#
# Add the following line to /etc/services and make a note of the port number you choose. Make sure the one you pick does
# not exist and is within the range of ports available.
#
# browse		63004/tcp   # Browser
# 
# Enable inetd: sudo systemctl enable inetd
#               sudo service inetd start
#
# In bpq32.cfg add the port specified above (63004) to the BPQ Telnet port list, CMDPORT= 
#
# Note the port's position offset in the list as that will be referenced in the APPLICATION line next.
# The first port in the CMDPORT= line is position 0 (zero), the next would be 1, then 2 and so on.
#
# Locate your APPLICATION line definitions in bpq32.cfg and another one next in the sequence. Make
# sure it has a unique <app number> between 1-32.
#
# Syntax: 
# APPLICATION <app number 1-32>,<node command>,<node instruction>
#
# APPLICATION 25,WEB,C 10 HOST 1 S
#
# <node instruction> is where the command 'web' is told to use BPQ port 10 (the telnet port, yours may be different!)
# HOST is a command to tell BPQ to look at the BPQ Telnet CMDPORT= list, and '1' is to pick offset position 1, that
# in turn resolves to TCP port 63004. The 'S' tells the node to return the user back to the node when they exit
# the web portal instead of disconnecting them, it refers to the word 'Stay'.

# Further config is at the bottom of the file for customization of the menu options as well as welcome message.
##### End of Config - Do not change anything below here.
#
# Global Vars
LinkRegex='[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,7}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)'
QuitCommandRegex='^(0|q|Q)$'
MenuCommandRegex='^(m|M)$' 
ListCommandRegex='^(l|L)$'
BackCommandRegex='^(B|b)$'
NewCommandRegex='^(n|N)$'
RedisplayCommandRegex='^(r|R)$'
WarningLimit=30
BackPage="none"
Referrer="none"
declare -A GlobalLinksArray # I'm an associative array!

set -e
trap 'catch $? $LINENO' EXIT
catch() {
	    if [ "$1" != "0" ]; then
		        # error handling goes here
				exit
      fi
 }




function CheckURLSanity() {
	local CheckURL=$1
	local ContentType
	local ContentTypeRegex

	# Ignore case with ,, below
	if ! [[ ${CheckURL,,} =~ $LinkRegex ]]
	then 
	    ReturnVal="Error: Address provided is not a valid URL"
	    return 1
	fi

	# Ignore case with ,, below
	if [[ ${CheckURL,,} =~ ^(gopher:|mailto:|ftp:|file:).*$ ]]
	then
		ReturnVal="Error: That type of URL is not allowed. Supported: http or https."
		return 1
	fi

	if $CurlBin -H "${UserAgent}" --output /dev/null --silent --head --fail "${CheckURL}"; then
		ContentType=$($CurlBin -H "${UserAgent}" -s -L -I --head -XGET "${CheckURL}" --output /dev/null -w '%{content_type}\n')
		echo "Content: $ContentType"
		ContentTypeRegex='^(text\/html|text\/plain).*$'
		if  ! [[ $ContentType =~ $ContentTypeRegex ]] 
		then
			ReturnVal="Error: Sorry, that content is not text!"
			return 1
		fi
		return 0
 	else
		ReturnVal="Error: Sorry, ${CheckURL} does not appear to exist."
		return 1
	fi	

}

function Quit() {
	echo "Exiting... Bye!"
	exit
}

function Begin() {
	local Address
	local URL
	local URLFix
	local Text

	Referrer="Begin"
	echo "Enter a web address (Q = quit, M = main menu):"
	read Address

	# Trim Input
	URLFix=${Address//[$'\t\r\n']} && Address=${Address%%*( )}

	if [[ $URLFix =~ $QuitCommandRegex ]]
	then
		Quit
		exit
	elif [[ $URLFix =~ $MenuCommandRegex ]]
	then
		MainMenu
	elif [[ $URLFix =~ ^https?:\/\/ ]]
	then
		URL="${URLFix}"
	else
		URL="http://${URLFix}"
	fi

	echo "Requesting ${URL}..."

	# Update Last Page Global
	BackPage="${URL}"

	LogUser "${Address}"
	GetPage "${URL}" "Begin" "${URL}"
}

function GetPage() {
	local URL
	local ReferrerFunc
	local ReferrerURL
	local Links

	URL=$1
	ReferrerFunc=$2
	ReferrerURL=$3

	# Generate Initial Page
	echo "One moment please..."
	DownloadPage "${URL}" "${ReferrerFunc}" "${ReferrerURL}"
	Text=$GlobalText # Global Conversion
	Links=$GlobalLinks # Global Conversion

	# Display The Page
	LineCount=0
	OldIFS=$IFS
	IFS='\n'
	for i in $Text
	do
		LineCount=$((LineCount+1))
	done
	IFS=$OldIFS

	if [ $LineCount -gt $WarningLimit ]
	then
		echo "This page is ${LineCount} lines long, are you sure you want to continue? (Y/n)"
			unset AskThem
			local AskThem
			read AskThem
			AskThemClean=${AskThem//[$'\t\r\n']} && AskThem=${AskThem%%*( )}
			if ! [[ $AskThemClean =~ ^(Y|y).*$ ]]
			then
				echo "Okay lets not do that then..."
				Prompt "${URL}"
			fi
	fi

	echo -e "Displaying ${URL}"
	echo -e "${Text}"
	echo -e "The previous page was: ${BackPage}"

	# Prompt Menu
	Prompt "${URL}" 
}

function LogUser() {
	local URL
	local Date

	URL=$1
	Date=`date`

	if ! [ -e ${WebLogFile} ]
	then
		touch ${WebLogFile}
	fi
	echo "${Date}: ${Callsign} requested ${URL}" >> ${WebLogFile}
}

function DownloadPage() {
	local URL
	local ReferrerFunc
	local ReferrerURL
	URL=$1
	ReferrerFunc=$2
	ReferrerURL=$3

	# Sanity Check the URL
	if ! CheckURLSanity "${URL}"
	then 
		echo $ReturnVal
		unset ReturnVal
		$ReferrerFunc "${ReferrerURL}"
	fi

        local Text
	Text=`$LynxBin -useragent=SimplePktPortal_L_y_n_x -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -trim_input_fields -trim_blank_lines -justify -dump  ${URL} | sed '/^$/d'`

	local Links
	Links=`${LynxBin} -useragent=SimplePktPortal_L_y_n_x -hiddenlinks=ignore -dump -unique_urls -listonly ${URL}`

	# Compile list of results into an array.
	local OldIFS
	OldIFS=$IFS
	IFS=$'\n'

	# SOME Pages will return links that Lynx will skip over yet still increments the Lynx link number displayed...
	# This logic sets the links into an array where the index of the array is identical to the link number Lynx displayed.
	local IndexRegex
	local HttpRegex
	GlobalLinkList=""
	for Results in ${Links}
	do
		IndexRegex='^((\ )+|)+([0-9]+)' # Bleugh
		HttpRegex='(https?.*)' # Barf
		[[ $Results =~ $IndexRegex ]] && IndexID=`echo ${BASH_REMATCH[0]} | xargs`  # Trim the whitepaces
		[[ $Results =~ $HttpRegex ]] && HttpURL=${BASH_REMATCH[0]} # It's an URL baby.
		if ! [ -z $IndexID ] # There's always one, grrr. Lynx returns a line "References" before the link list...
		then
			GlobalLinksArray[$IndexID]=$HttpURL
			# Build the human readable list of links
			GlobalLinkList+="$IndexID = $HttpURL\n"

		fi
	done
	IFS=$OldIFS

	# The Globals:
	# GlobalLinksArray
	# GlobalLinkList
	GlobalText="${Text}"
}

function Prompt() {
	local URL
	local RestartURL

	URL=$1
	RestartURL=$1

	# Prompt
	local MyLinkID
	echo "Enter Link Number: (Q = quit, L = list links, B = back, N = open new link, M = main menu, R = Redisplay Page)"
	read MyLinkID

	# Trim Input
	local LinkIDFix
	LinkIDFix=${MyLinkID//[$'\t\r\n']} && MyLinkID=${MyLinkID%%*( )}

	# Handle Link Choice
	local LinkIDRegex
	LinkIDRegex='^([0-9])+$' 

	if [[ $LinkIDFix =~ $QuitCommandRegex ]] 
	then
		Quit
	elif [[ $LinkIDFix =~ $ListCommandRegex ]] 
	then
		LinkCount=${#GlobalLinksArray[*]}
		if [ $LinkCount -gt $WarningLimit ]
		then 
			echo "The link list is ${LinkCount} lines long, are you sure you want to continue? (Y/n)"
			unset AskThem
			local AskThem
			read AskThem
			AskThemClean=${AskThem//[$'\t\r\n']} && AskThem=${AskThem%%*( )}
			if ! [[ $AskThemClean =~ ^(Y|y).*$ ]]
			then
				echo "Okay lets not do that then..."
				Prompt "${URL}"
			fi
		fi
		echo -e ${GlobalLinkList} # Display Links
		Prompt "${URL}" # Prompt
	elif [[ $LinkIDFix =~ $NewCommandRegex ]] 
	then
		unset GlobalLinkList
		unset GlobalLinkArray
		unset GlobalText
		Begin
	elif [[ $LinkIDFix =~ $RedisplayCommandRegex ]] 
	then
		echo -e "Redisplaying Page..."
		echo -e "$GlobalText"
		Prompt "${URL}"
	elif [[ $LinkIDFix =~ $MenuCommandRegex ]] 
	then
		unset GlobalLinkList
		unset GlobalLinkArray
		unset GlobalText
		MainMenu
	elif [[ $LinkIDFix =~ $BackCommandRegex ]]
	then
		if [[ ${BackPage} == "none" ]]
		then
			echo "Error: We can't go there!"
			Prompt "${URL}" # Again
			exit
		else
			unset GlobalLinkList
			unset GlobalLinkArray
			unset GlobalText
			GetPage "${BackPage}" "Prompt" "${URL}"
		fi
	elif  [[ $LinkIDFix =~ $LinkIDRegex ]] 
	then
		# So an actual link ID has been requested...
		# LinkRegex is Global
		local LinkURL 

		LinkURL=${GlobalLinksArray[${LinkIDFix}]}
		if ! [[ $LinkURL =~ $LinkRegex ]]
		then 
			echo "Error: Sorry, ${LinkURL} cannot be accessed via this portal."
			Prompt "${URL}" # Again
		fi
		
		# Update Back-Page Global
		BackPage="${LinkURL}"

		unset GlobalLinkList
		unset GlobalLinkArray
		unset GlobalText
		LogUser "${LinkURL}"
		GetPage "${LinkURL}" "Prompt" "${URL}"
	else
		echo "Error: Oops! Invalid Link Number."
		Prompt "${URL}" # Again
		exit
	fi

}


function MainMenu() {
#	echo "This portal is a work in progress. Please report bugs to pe1rrr@pe1rrr.#nbw.nld.euro"
	echo ""
	echo "[1] Enter your own web address"
	echo ""
	echo "Bookmarks"
	echo "[100] W6EK.org Sierra Foothills ARC"
	echo "[200] craiger.org/bbs info about this node"
	echo ""

	echo "Enter Choice: (Q = quit)"

	local IDRegex
	local Choice
	local URL

	read Choice

	# Trim
	Selection=${Choice//[$'\t\r\n']} && Choice=${Choice%%*( )}
	IDRegex='^([0-9]|q|Q)+$'

	if ! [[ $Selection =~ $IDRegex ]]
	then 
	    echo "Error: Sorry, that selection was not valid, please check and try again."
	    MainMenu
	    exit
	fi

	if [[ $Selection =~ $QuitCommandRegex ]]
	then
		Quit
		exit
	fi

	if [ $Selection -eq 0 ] 
	then
		exit
	elif [ $Selection -eq 1 ]
	        then Begin
		exit
	elif [ $Selection -eq 100 ]
	then
		URL="http://w6ek.org/"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 101 ]
	then
		URL="https://rsgb.org"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 200 ]
	then
		URL="http://craiger.org/bbs.html"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 202 ]
	then
		URL="https://www.amsat.org"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 400 ]
	then
		URL="https://www.rijksoverheid.nl/onderwerpen/coronavirus-covid-19"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 401 ]
	then
		URL="https://veron.nl"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 402 ]
	then
		URL="https://www.VRZA.nl/wp"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 403 ]
	then
		URL="https://dares.nl"
		GetPage "${URL}" "MainMenu"
	else
		echo "Error: Sorry, you must make a selection from the menu!"
		MainMenu
	fi


}

function WelcomeMsg() {
	local Callsign
	Callsign=$1
#	echo "Hello ${Callsign}, welcome to Simple Packet Web - by PE1RRR Version ${Version}"
#	echo "All requests are logged, inappropriate sites are blocked by OpenDNS FamilyShield"
	return 0
}

function CheckCallsign() {
	local Call
	local CallsignRegex

	Call=$1
	CallsignRegex="[a-zA-Z0-9]{1,3}[0123456789][a-zA-Z0-9]{0,3}[a-zA-Z]"

	if [[ $Call =~ $CallsignRegex ]] 
	then
		return 0
	else
		return 1
	fi
}


# Inetd Connectivity- BPQ Node Connect and Telnet IP connect are handled differently.
Client=$1
if [ -z $Client ] 
then
	echo "Misconfigured, please ensure this script is called with the 'client <ip/ax25>' argument from inetd"
	exit
	fi

if [ ${Client} == "ip" ]
then
	# Connection from 2nd param of inetd after 'client' on 'ip' port so standard telnet user is prompted for a callsign.
	echo "Please enter your callsign:"
	read CallsignIn
elif	[ ${Client} == "ax25" ]
then
	# Connection came from a linbpq client on 'ax25' port which by default sends callsign on connect.
	read CallsignIn
fi

# Trim BPQ-Term added CRs and Newlines.
Callsign=${CallsignIn//[$'\t\r\n']} && CallsignIn=${CallsignIn%%*( )}
# Get rid of SSIDs
CallsignNOSSID=`echo ${Callsign} | cut -d'-' -f1`


# Check Validity of callsign
if ! CheckCallsign "${CallsignNOSSID}"
then
	echo "Error: Invalid Callsign..."
	Quit
	exit
fi

WelcomeMsg "${CallsignNOSSID}"
MainMenu
