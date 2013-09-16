#!/bin/sh

prevminute=$(date '+%b %d %H:%M' -d "1 minutes ago")
echo "$prevminute"

subject='Processes Monitoring'
body=$(grep $prevminute /var/log/syslog | grep "Monitor")
emailto=''

$(echo $body | mail -s $subject $emailto)


