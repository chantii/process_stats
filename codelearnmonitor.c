#include<stdio.h>
#include<unistd.h>
#include"./codelearnmonitor.h"

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

void process_processes(){
  PROCTAB* proc  = openproc(PROC_FILLMEM | PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS);
  proc_t proc_info;
  memset(&proc_info, 0, sizeof(proc_info));
  while(readproc(proc, &proc_info) != NULL) {
   struct cur_process *cpp = find_process(proc_info.tid);
   if(cpp != NULL){
     time_t prev_time = cpp->timestamp;
     time_t cur_time = getCurrentTime();
     double diff_in_seconds = difftime(cur_time,prev_time);
     double cpu_percentage = (proc_info.cutime + proc_info.cstime - cpp->cutime + cpp->cstime)/ diff_in_seconds;
     unsigned memory = proc_info.vm_size;
     printf("%5d\t%20s\t%20s\t%.f\t%5u\t%10lld\n",proc_info.tid, proc_info.suser, proc_info.cmd, cpu_percentage,memory,proc_info.start_time/sysconf(_SC_CLK_TCK)); 
     if(memory >= 1028*1028*2)
     printf("Memory has increased by %5d,%20s,%20s,%5u\n",proc_info.tid, proc_info.suser, proc_info.cmd,memory); 
   }
   struct cur_process *cp = malloc(sizeof(struct cur_process));
   cp->pid = proc_info.tid;
   cp->cutime = proc_info.cutime;
   cp->cstime = proc_info.cstime;
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
  process_processes();
  sleep(5);
  process_processes();
}
