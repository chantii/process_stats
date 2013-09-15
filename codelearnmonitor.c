#include<stdio.h>
#include<unistd.h>
#include"codelearnmonitor.h"
#include<syslog.h>

void add_process(struct cur_process *cp){
  HASH_ADD_INT(processes, pid, cp);
}

struct cur_process *find_process(int pid ){
  struct cur_process *cp;
  HASH_FIND_INT(processes, &pid, cp);
  return cp;
}

void delete_process(struct cur_process *process){
  HASH_DEL(processes, processes);
}

time_t getCurrentTime(){
  time_t now;
  time(&now);
  return now;
}

void checkCPUPLimit(double cpup_limit, double cur_usage, char* username, char* command, int pid){
    if(cur_usage >= cpup_limit){
        //printf("Exceeded cpu limit of %d \n", pid);
        openlog("Monitor", LOG_PID|LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "CPU has reached threshold: %f,%f,%s,%s,%d", cpup_limit, cur_usage, username, command, pid);
        closelog();
    }
}

void checkMemoryLimit(unsigned memory_limit, unsigned cur_usage, char* username, char* command, int pid){
    if(cur_usage >= memory_limit){
        //printf("Exceeded Memory Limit of %d \n", pid);
        openlog("Monitor", LOG_PID|LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Memory has reached threshold: %d,%d,%s,%s,%d", memory_limit, cur_usage, username, command, pid);
        closelog();
    }
}

void process_processes(double cpup_limit, unsigned memory_limit){
  PROCTAB* proc  = openproc(PROC_FILLMEM | PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS);
  proc_t proc_info;
  memset(&proc_info, 0, sizeof(proc_info));
  while(readproc(proc, &proc_info) != NULL) {
   struct cur_process *cpp = find_process(proc_info.tid);
   if(cpp != NULL){
     time_t prev_time = cpp->timestamp;
     time_t cur_time = getCurrentTime();
     double diff_in_seconds = difftime(cur_time,prev_time);
     long unsigned int pid_diff = (proc_info.utime + proc_info.stime) - (cpp->utime + cpp->stime);
     double cpu_percentage = pid_diff / diff_in_seconds;
     unsigned memory = proc_info.vm_size;

     checkCPUPLimit(cpup_limit, cpu_percentage, proc_info.suser, proc_info.cmd, proc_info.tid);
     checkMemoryLimit(memory_limit, memory, proc_info.suser, proc_info.cmd, proc_info.tid );
   }

   struct cur_process *cp = malloc(sizeof(struct cur_process));
   cp->pid = proc_info.tid;
   cp->cutime = proc_info.cutime;
   cp->cstime = proc_info.cstime;
   cp->utime = proc_info.utime;
   cp->stime = proc_info.stime;
   cp->user = proc_info.suser;
   cp->command = proc_info.cmd;
   cp->vm_size = proc_info.vm_size;
   cp->timestamp = getCurrentTime();
   add_process(cp);
   // printf("%5d\t%20s:\t%5lld\t%5lu\t%s\n",proc_info.tid, proc_info.cmd, proc_info.utime + proc_info.stime, proc_info.vm_size, proc_info.suser);
 }
  closeproc(proc);
}

int main(int argc, char** argv){
  if(argc == 3){
  double cpup_limit = atof(argv[1]);
  unsigned memory_limit = atoi(argv[2]);
  //printf("memory limit = %d, cpu limit = %f \n ",memory_limit, cpup_limit);
  while(1){
     process_processes(cpup_limit, memory_limit);
        sleep(1);
    }
  }else{
    printf("Usage: monitor <cpu_percentage> <memory_in_kb>\n");
  }
}
