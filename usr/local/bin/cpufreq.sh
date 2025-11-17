#!/bin/sh

#temp=`head -n 1 /sys/class/thermal/thermal_zone0/temp | xargs -I{} awk "BEGIN {printf \"%.2f\n\", {}/1000}"`
#echo $((`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq`/1000)) MHz, $temp degrees
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 
cat /sys/class/thermal/thermal_zone0/temp 
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor



