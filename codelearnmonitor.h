#include<proc/readproc.h>
#include "uthash.h"
#include<time.h>

struct cur_process{
  int pid;
  unsigned long long cutime;
  unsigned long long cstime;
  unsigned long long utime;
  unsigned long long stime;
  char* user;
  char* group;
  char* command;
  unsigned vm_size;
  time_t timestamp;
  UT_hash_handle hh;
};

struct user_stat{
  char username[10];
  int nproc;
  UT_hash_handle hh;
};

struct limit_conf{
  int rss;
  double cpu;
  int nproc;
};

struct cur_process *processes = NULL;
struct user_stat *userstats = NULL;

void add_process(struct cur_process *cp);
struct cur_process *find_process(int pid);
void delete_process(struct cur_process *process);

void add_userstat(struct user_stat *us);
struct user_stat *find_userstat(char* username);
void delete_userstat(struct user_stat *us);

void process_processes();
time_t getCurrentTime();

void dumpProcessDetails(char* command);
void checkCPUPLimit(double cpup_limit, double cur_usage, char* username, char* command, int pid);
void checkMemoryLimit(unsigned memory_limit, unsigned cur_usage, char* username, char* command, int pid);

void countAndValidateNProc(char* username, int maxNProc);
void parse_limits_conf_file(struct limit_conf *lc, char* name);

int isUserOfGroup(char* username, char* groupname);
