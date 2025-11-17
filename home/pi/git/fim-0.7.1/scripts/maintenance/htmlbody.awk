#!/usr/bin/awk -f
#
# This program extracts the body of an HTML file.
#
BEGIN { sp=0 }
/^.\/\<body\>/ { sp=sp+1; }
/.*/ { if(sp==1){ gsub("&minus;","-"); print; } }
/^.\<body\>/ { sp=sp+1; }

