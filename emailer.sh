#!/bin/sh
prevminute=$(date '+%b %d %H:%M' -d "1 minutes ago")
echo "$prevminute"

subject='Processes Monitoring'
emailto=''

grep "$prevminute" /var/log/syslog | grep "Monitor" > /tmp/monitoremail.txt

if [ -s "/tmp/monitoremail.txt" ]
then
    echo "Sending email"
    cat /tmp/monitoremail.txt | mail -s $subject $emailto
else
    echo "File is Empty, No Email"
fi
