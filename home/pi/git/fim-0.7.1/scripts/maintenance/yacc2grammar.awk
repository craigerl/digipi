#!/usr/bin/awk -f
#
# This program prints out the Grammar section of a yacc.output file. 
#
BEGIN { sp=0 }
/program:/ { sp=sp+1; }
/^Grammar/ { sp=sp+1; }
/^Terminals/ { sp=sp+1; }
#/.*/ { if(sp==3){  print; } }
/.*/ { if(sp==3){ gsub("^[ \t]*[0-9][0-9]*","\t"); print; } }
