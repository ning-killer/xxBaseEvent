#if defined(LITEOS) || defined(WIN32)
void backtrace_init() {
  return;
}
#else

#include <dlfcn.h>
#include <stdlib.h>
#include <unwind.h>
#include <assert.h>
#include <stdio.h>
//#include <link.h>	   // required for __ELF_NATIVE_CLASS
#include <bits/wordsize.h>  // required for (__ELF_NATIVE_CLASS)__WORDSIZE
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h> // SYS_gettid
#include <unistd.h>
#include <sys/prctl.h>
#include "log/log/log_client.h"
#include "eventservice/base/common.h"

#if __WORDSIZE == 32
# define WORD_WIDTH 8
#else
/* We assyme 64bits.  */
# define WORD_WIDTH 16
#endif

#define BACKTRACE_DUMPTO_FILE true
#define BACKTRACE_FILE_TOT 3
//const char *bt_file_format = "/tmp/back_tace/out%d";

#if defined(UBUNTU64)
const char *bt_file_format =  "/tmp/bt_file_%d.txt";
#elif defined(LITEOS)
const char *bt_file_format = "bt_file_%d.txt";
#else
const char *bt_file_format = "/mnt/log/system_server_files/bt_file%d.txt";
#endif

struct trace_arg {
  void  **array;
  int     cnt;
  int     size;
};

static _Unwind_Reason_Code
backtrace_helper(struct _Unwind_Context *ctx, void *a) {
  struct trace_arg *arg = (struct trace_arg *)a;

  // We are first called with address in the __backtrace function. Skip it.
  if (arg->cnt != -1) {
    arg->array[arg->cnt] = (void *)_Unwind_GetIP(ctx);
  }
  if (++arg->cnt == arg->size) {
    return _URC_END_OF_STACK;
  }
  return _URC_NO_REASON;
}

char **backtrace_symbols(void *const *array, int size) {
  Dl_info info[size];
  int status[size];
  int cnt;
  size_t total = 0;
  char **result;

  // Fill in the information we can get from `dladdr'.
  for (cnt = 0; cnt < size; ++cnt) {
    status[cnt] = dladdr(array[cnt], &info[cnt]);
    if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
      // We have some info, compute the length of the string which will be
      // "<file-name>(<sym-name>) [+offset].
      total += (strlen(info[cnt].dli_fname ? : "")
                + (info[cnt].dli_sname ? strlen(info[cnt].dli_sname) + 3 + WORD_WIDTH + 3 : 1)
                + WORD_WIDTH + 5);
    } else {
      total += 5 + WORD_WIDTH;
    }
  }

  // Allocate memory for the result.
  result = (char **)malloc(size * sizeof(char *) + total);
  if (result != NULL) {
    char *last = (char *)(result + size);
    for (cnt = 0; cnt < size; ++cnt) {
      result[cnt] = last;
      if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0') {
        char buf[20];
        if (array[cnt] >= (void *)info[cnt].dli_saddr) {
          sprintf(buf, "+%#lx",
                  (unsigned long)((unsigned long)array[cnt] - (unsigned long)info[cnt].dli_saddr));
        } else {
          sprintf(buf, "-%#lx",
                  (unsigned long)((unsigned long)info[cnt].dli_saddr - (unsigned long)array[cnt]));
        }

        last += 1 + sprintf(last, "%s%s%s%s%s[%p]",
                            info[cnt].dli_fname ? : "",
                            info[cnt].dli_sname ? "(" : "",
                            info[cnt].dli_sname ? : "",
                            info[cnt].dli_sname ? buf : "",
                            info[cnt].dli_sname ? ") " : " ",
                            array[cnt]);
      } else {
        last += 1 + sprintf(last, "[%p]", array[cnt]);
      }
    }
    assert(last <= (char *)result + size * sizeof(char *) + total);
  }

  return result;
}

// Perform stack unwinding by using the _Unwind_Backtrace.
int backtrace(void **array, int size) {
  struct trace_arg arg;
  arg.array = array;
  arg.size = size;
  arg.cnt = -1;
  if (size >= 1) {
    _Unwind_Backtrace(backtrace_helper, &arg);
  }
  return arg.cnt != -1 ? arg.cnt : 0;
}

int backtrace_file_dump(int num, int nptrs, char **strings, char *thread_name) {
  char filename[100];
  FILE *fp = NULL;
  int idx = BACKTRACE_FILE_TOT - 1;
  time_t cur_t;

  for(int i = 0; i < BACKTRACE_FILE_TOT; i++) {
    sprintf(filename, bt_file_format, i);
    struct stat st;
    int err = stat(filename, &st);
    if(err) {
      idx = i;
      break;
    }
    if(i == 0) {
      idx = i;
      cur_t = st.st_mtim.tv_sec;
    } else {
      if(cur_t > st.st_mtim.tv_sec) {
        idx = i;
        cur_t = st.st_mtim.tv_sec;
      }
    }
  }

  sprintf(filename, bt_file_format, idx);
  fp = fopen(filename, "w");
  if(fp == NULL) {
    return -1;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_t secs = tv.tv_sec;
  struct tm *tp = localtime(&secs);

  fprintf(fp, "============================================\n"
          "System signal \"SIGSEGV\" received, thread:%ld, name:%s\n",
          syscall(SYS_gettid), thread_name);
  fprintf(fp, "system_time:%u/%u/%u-%u:%u:%u\n",
          tp->tm_year+1900u, tp->tm_mon + 1U, tp->tm_mday,
          tp->tm_hour, tp->tm_min, tp->tm_sec);
  fprintf(fp, "backtrace() returned %d addresses\n", nptrs);

  // The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
  // would produce similar output to the following.
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < nptrs; i++) {
    fprintf(fp, "%s\n", strings[i]);
  }
  fclose(fp);
  return  0;
}

void signal_action(int num) {
#define SIZE 100
  void *buffer[SIZE];
  char **strings;
  char thread_name[100] = {0};

  prctl(PR_GET_NAME, thread_name);
#ifndef __FACE__
  LOG_TRACE(LT_SYS, "%04d System signal \"SIGSEGV\" received, thread:%ld, name:%s\n",
            LOG_ID_CRASHED_SIGSEGV, syscall(SYS_gettid), thread_name);
#endif
  Log_FlushCache();
  vzsleep(2*1000);
  printf("\n\n============================================\n"
         "System signal \"SIGSEGV\" received, thread:%ld, name:%s\n",
         syscall(SYS_gettid), thread_name);
  int nptrs = backtrace(buffer, SIZE);
  printf("backtrace() returned %d addresses\n", nptrs);

  // The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
  // would produce similar output to the following.
  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < nptrs; i++) {
    printf("%s\n", strings[i]);
  }
  if(BACKTRACE_DUMPTO_FILE) {
    backtrace_file_dump(num, nptrs, strings, thread_name);
  }
  fflush(stdout);
  free(strings);
}

void backtrace_init() {
  struct sigaction act, oldact;
  act.sa_handler = signal_action;
  act.sa_flags = SA_NODEFER | SA_RESETHAND;
  sigaction(SIGSEGV, &act, &oldact);
}
#endif
