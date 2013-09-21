#include<stdio.h>
#include<unistd.h>
#include"codelearnmonitor.h"
#include<syslog.h>
#include<ctype.h>
#include<grp.h>

#define LIMITS_DEF_USER     0 /* limit was set by an user entry */
#define LIMITS_DEF_GROUP    1 /* limit was set by a group entry */
#define LIMITS_DEF_DEFAULT  2 /* limit was set by an default entry */
#define LIMITS_DEF_NONE     3 /* this limit was not set yet */
#define LINE_LENGTH 1024

void add_process(struct cur_process *cp){
    HASH_ADD_INT(processes, pid, cp);
    //printf("Adding Process %d \n", cp->pid);
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
    printf("CPU validation for %s \n ", username);
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
    printf("Memory checking %s \n ", username);
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
    printf("NPROC validation for %s \n ", username);
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

void parse_limits_conf_file(struct limit_conf *lc, char* name){
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
        if(strcmp(name,domain) == 0){
            if(strcmp(item,"rss") == 0)
                lc->rss = atoi(value);
            if(strcmp(item,"cpu") == 0)
                lc->cpu = atof(value);
            if(strcmp(item,"nproc") == 0)
                lc->nproc = atoi(value);
        }
    }
    fclose(fil);
}

int isUserOfGroup(char* username, char* groupname){
    int numgroups, iter, retCode;
    int numgroups_max = sysconf(_SC_NGROUPS_MAX) + 1;
    gid_t groupList[numgroups_max];
    retCode = initgroups(username, 0);

    if (retCode != 0){
        printf("Permissions issue : %d\n\n", retCode);
        return -1;
    }
    numgroups = getgroups(numgroups_max, groupList);
    for (iter=0; iter <= numgroups; iter++){
        if (iter != 0 &&  iter != numgroups ){
            struct group *g = getgrgid(groupList[iter]);
            if(g != NULL){
                if(strcmp(g->gr_name,groupname) == 0){
                    return 1;
                }
            }
        }
    }
    return 0;
}

void process_processes(double cpup_limit, unsigned memory_limit, int maxNProc, char* name){
    PROCTAB* proc  = openproc(PROC_FILLMEM | PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS);
    userstats = NULL;
    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));
    while(readproc(proc, &proc_info) != NULL ) {
        if(strcmp(proc_info.suser,"root") != 0){
            struct cur_process *cpp = find_process(proc_info.tid);
            if(cpp != NULL){
                time_t prev_time = cpp->timestamp;
                time_t cur_time = getCurrentTime();
                double diff_in_seconds = difftime(cur_time,prev_time);
                long unsigned int pid_diff = (proc_info.utime + proc_info.stime) - (cpp->utime + cpp->stime);
                double cpu_percentage = pid_diff / diff_in_seconds;
                unsigned memory = proc_info.vm_rss;

                if(cpup_limit > 0 && (strcmp(cpp->user,name) == 0 || isUserOfGroup(cpp->user,name)))
                    checkCPUPLimit(cpup_limit, cpu_percentage, proc_info.suser, proc_info.cmd, proc_info.tid);
                if(memory_limit >0 && (strcmp(cpp->user,name) == 0 || isUserOfGroup(cpp->user, name)))
                    checkMemoryLimit(memory_limit, memory, proc_info.suser, proc_info.cmd, proc_info.tid );
            }else{
                //printf("No Old process found \n");
            }

            struct cur_process *cp = malloc(sizeof(struct cur_process));
            cp->pid = proc_info.tid;
            cp->cutime = proc_info.cutime;
            cp->cstime = proc_info.cstime;
            cp->utime = proc_info.utime;
            cp->stime = proc_info.stime;
            cp->user = proc_info.suser;
            cp->command = proc_info.cmd;
            cp->vm_size = proc_info.vm_rss;
            cp->group = proc_info.rgroup;
            cp->timestamp = getCurrentTime();
            add_process(cp);

            //printf("Group Name %s \n", proc_info.fgroup);
            if(maxNProc > 0 && (strcmp(cp->user,name) == 0 || isUserOfGroup(cp->user,name)))
                countAndValidateNProc(proc_info.suser, maxNProc);
            //printf("%5d\t%20s:\t%5lld\t%5lu\t%s\n",proc_info.tid, proc_info.cmd, proc_info.utime + proc_info.stime, proc_info.vm_size, proc_info.suser);
        }
    }
    closeproc(proc);
}

int main(int argc, char** argv){
    if(argc == 2){
        struct limit_conf *lc = malloc(sizeof(struct limit_conf));
        parse_limits_conf_file(lc, argv[1]);
        double cpup_limit = lc->cpu;
        unsigned memory_limit = lc->rss;
        int maxNProc = lc->nproc;
        char* name = argv[1];
        printf("memory limit = %d, cpu limit = %f, nproc = %d \n ",memory_limit, cpup_limit, maxNProc);
        while(1){
            process_processes(cpup_limit, memory_limit, maxNProc, name);
            sleep(1);
        }
    }else{
        printf("Usage: monitor <Group_or_User_name>\n");
    }
}
