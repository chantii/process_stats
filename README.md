process_stats
=============

Monitors Processes in Linux Machine and logs in syslog incase of threshold limitations.

How to install
==============
1. Install procps and uthash libraries

  Ubuntu : <br/>
  apt-get install procps-dev <br/>
  apt-get install uthash-dev <br/>

2. Pull the source code.
3. Compile the code using command
  gcc -o monitor codelearnmonitor.c -lproc

Syntax
======
monitor cpu_percentage_limit memory_limit_in_kb

Logging
=======
Syslog will be updated, whenever a process exceeds the limits given in arguments.

Log Format:  memory_limit, cur_usage, username, command, pid

