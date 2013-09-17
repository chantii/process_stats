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
  char* command;
  unsigned vm_size;
  time_t timestamp;
  UT_hash_handle hh;
};

struct cur_process *processes = NULL;

void add_process(struct cur_process *cp);
struct cur_process *find_process(int pid);
void delete_process(struct cur_process *process);

void process_processes();
time_t getCurrentTime();

void dumpProcessDetails(char* command);
void checkCPUPLimit(double cpup_limit, double cur_usage, char* username, char* command, int pid);
void checkMemoryLimit(unsigned memory_limit, unsigned cur_usage, char* username, char* command, int pid);

