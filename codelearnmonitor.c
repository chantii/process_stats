#include<stdio.h>
#include<unistd.h>
#include"codelearnmonitor.h"
#include<syslog.h>
#include <ctype.h>

#define LIMITS_DEF_USER     0 /* limit was set by an user entry */
#define LIMITS_DEF_GROUP    1 /* limit was set by a group entry */
#define LIMITS_DEF_DEFAULT  2 /* limit was set by an default entry */
#define LIMITS_DEF_NONE     3 /* this limit was not set yet */
#define LINE_LENGTH 1024

void add_process(struct cur_process *cp){
  HASH_ADD_INT(processes, pid, cp);
}

struct cur_process *find_process(int pid ){
  struct cur_process *cp;
  HASH_FIND_INT(processes, &pid, cp);
  return cp;
}

void delete_process(struct cur_process *process){
  HASH_DEL(processes, process);
}

void add_userstat(struct user_stat *us){
  HASH_ADD_STR(userstats, username, us);
}

struct user_stat *find_userstat(char* username){
  struct user_stat *us;
  HASH_FIND_STR(userstats, username, us);
  return us;
}

void delete_userstat(struct user_stat *us){
  HASH_DEL(userstats, us);
}

time_t getCurrentTime(){
  time_t now;
  time(&now);
  return now;
}

void dumpProcessDetails(char* cmd) {
    char buffer[5000];
    FILE *pipe;
    int len;
    pipe = popen(cmd, "r");
    if (NULL == pipe) {
        perror("pipe");
        exit(1);
    }
    else{
        while (!feof(pipe)){
        if(fgets(buffer, sizeof(buffer), pipe) != NULL){
            //printf("%s", buffer);
            syslog(LOG_INFO, "%s", buffer );
         }
        }
    }
    pclose(pipe);
}

void checkCPUPLimit(double cpup_limit, double cur_usage, char* username, char* command, int pid){
    if(cur_usage >= cpup_limit && strcmp(username,"root") != 0 ){
        //printf("Exceeded cpu limit of %d \n", pid);
        openlog("Monitor", LOG_PID|LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "[MONITOR_CPU_EXCEEDED] CPU has reached threshold: %f,%f,%s,%s,%d", cpup_limit, cur_usage, username, command, pid);
        char pscommand[20] = "ps -ef|grep ";
        char pidstring[10];
        sprintf(pidstring, "%d", pid);
        strcat(pscommand, pidstring);
        //printf("pscommand is %s ", pscommand);
        dumpProcessDetails(pscommand);
        closelog();
    }
}

void checkMemoryLimit(unsigned memory_limit, unsigned cur_usage, char* username, char* command, int pid){
    if(cur_usage >= memory_limit && strcmp(username,"root") != 0){
        //printf("Exceeded Memory Limit of %d \n", pid);
        openlog("Monitor", LOG_PID|LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "[MONITOR_MEMORY_EXCEEDED] Memory has reached threshold: %d,%d,%s,%s,%d", memory_limit, cur_usage, username, command, pid);
        char pscommand[20] = "ps -ef|grep ";
        char pidstring[10];
        sprintf(pidstring, "%d", pid);
        strcat(pscommand, pidstring);
        //printf("pscommand is %s ", pscommand);
        dumpProcessDetails(pscommand);
        closelog();
    }
}

void countAndValidateNProc(char* username, int maxNProc){
  //printf("Finding for %s \n ", username);
  struct user_stat *us = malloc(sizeof(struct user_stat));
  us = find_userstat(username);
  if(us != NULL){
      int currentNProc = us->nproc + 1;
      //printf("%s - %d \n ", username, currentNProc );
      if(currentNProc > maxNProc - 2){
        syslog(LOG_INFO, "[MONITOR_NPROC_LIMIT] Maximum allowed process for the user Exceeded: %s,%d", username, maxNProc);
      }
      us->nproc++;
      add_userstat(us);
  }else{
      struct user_stat *nus = malloc(sizeof(struct user_stat));
      //printf("Adding New User \n");
      strcpy(nus->username,username);
      nus->nproc = 1;
      add_userstat(nus);
  }
}

void parse_limits_conf_file(){
    FILE *fil;
    char buf[LINE_LENGTH];
    fil = fopen("/etc/security/limits.conf", "r");
    if (fil == NULL) {
        printf("Error Reading limits.conf file");
    }
    /* init things */
    memset(buf, 0, sizeof(buf));
    /* start the show */
    while (fgets(buf, LINE_LENGTH, fil) != NULL) {
        char domain[LINE_LENGTH];
        char ltype[LINE_LENGTH];
        char item[LINE_LENGTH];
        char value[LINE_LENGTH];
        int i,j;
        char *tptr;
        tptr = buf;

        /* skip the leading white space */
        while (*tptr && isspace(*tptr))
            tptr++;
        strncpy(buf, tptr, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        /* Rip off the comments */
        tptr = strchr(buf,'#');
        if (tptr)
            *tptr = '\0';
        /* Rip off the newline char */
        tptr = strchr(buf,'\n');
        if (tptr)
            *tptr = '\0';
        /* Anything left ? */
        if (!strlen(buf)) {
            memset(buf, 0, sizeof(buf));
            continue;
        }

        memset(domain, 0, sizeof(domain));
        memset(ltype, 0, sizeof(ltype));
        memset(item, 0, sizeof(item));
        memset(value, 0, sizeof(value));
        i = sscanf(buf,"%s%s%s%s", domain, ltype, item, value);
        for(j=0; j < strlen(domain); j++){
            domain[j]=tolower(domain[j]);
            printf("%c", domain[j]);
        }
            printf(",");
        for(j=0; j < strlen(ltype); j++){
            ltype[j]=tolower(ltype[j]);
            printf("%c", ltype[j]);
        }
            printf(",");
        for(j=0; j < strlen(item); j++){
            item[j]=tolower(item[j]);
            printf("%c", item[j]);
        }
            printf(",");
        for(j=0; j < strlen(value); j++){
          value[j]=tolower(value[j]);
          printf("%c", value[j]);
        }
            printf("\n");
  }
    fclose(fil);
}


void process_processes(double cpup_limit, unsigned memory_limit, int maxNProc){
  PROCTAB* proc  = openproc(PROC_FILLMEM | PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS);
  userstats = NULL;
  processes = NULL;
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
   countAndValidateNProc(proc_info.suser, maxNProc);
   //printf("%5d\t%20s:\t%5lld\t%5lu\t%s\n",proc_info.tid, proc_info.cmd, proc_info.utime + proc_info.stime, proc_info.vm_size, proc_info.suser);
 }
  closeproc(proc);
}

int main(int argc, char** argv){
parse_limits_conf_file();
/*
  if(argc == 4){
  double cpup_limit = atof(argv[1]);
  unsigned memory_limit = atoi(argv[2]);
  int maxNProc = atoi(argv[3]);
  printf("memory limit = %d, cpu limit = %f, nproc = %d \n ",memory_limit, cpup_limit, maxNProc);
  while(1){
     process_processes(cpup_limit, memory_limit, maxNProc);
     sleep(1);
    }
  }else{
    printf("Usage: monitor <cpu_percentage> <memory_in_kb> <max_nproc>\n");
  }
*/
}
