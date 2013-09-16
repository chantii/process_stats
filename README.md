Process Monitoring
==================

Monitors Processes in Linux Machine and logs in syslog incase of threshold limitations.

How to install
==============
1. Install procps and uthash libraries

  Ubuntu : <br/>
  <b>apt-get install libproc-dev</b> <br/>
  <b>apt-get install uthash-dev </b><br/>

2. Pull the source code.
3. Compile the code using command
  <b>gcc -o monitor codelearnmonitor.c -lproc </b>

Syntax
======
<b> monitor cpu_percentage_limit memory_limit_in_kb </b>

Logging
=======
Syslog will be updated, whenever a process exceeds the limits given in arguments.

Log Format:  memory_limit, cur_usage, username, command, pid

Testing
=======
Use statwatch.c to see cpu% and memory usage of all the processes running in the machine.
this is just like top command. 

Installer
=========
use insall.sh to install the utility in your Linux Machine

<b>sudo sh install.sh </b>

After installation you can use "monitor" utility to start monitoring the processes.
