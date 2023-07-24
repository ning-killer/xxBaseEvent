#include "mem_dump.h"

#ifdef MEM_TEST
typedef struct _malloc_inode {
  _malloc_inode      *next_;
  unsigned int       alloc_time_s;           // 分配时间(秒)
  unsigned int       alloc_time_us;          // 分配时间(微妙)
  char              func_name_[32];         // 分配文件
  unsigned int       func_line_;             // 分配行号
  unsigned int       mem_size_;              // 分配内存的size
  unsigned long int  thread_id_;             // 所属线程的ID
  char              mem_addr_[0];           // 用户使用地址
} MEM_INODE;

typedef struct _thread_header_inode {
  _thread_header_inode *next_;
  MEM_INODE         *fastmem;                // 当前线程最旧的mem
  MEM_INODE         *lastmem;                // 当前线程最新的mem
  unsigned long int thread_id_;               // 线程ID
  char             thread_name_[16];          // 线程名（线程名获取要在线程名被设置成功之后才可以）
  unsigned int      current_id_mem_total;      // 当前线程所有mem数
  unsigned int      current_id_max_mem_size;   // 分配过最大的一块内存的大小
  char             max_mem_size_func_name[32]; // 最大的那块内存的分配文件
  unsigned int      max_mem_size_func_line_;   // 最大的那块内存的分配行号
  unsigned int      current_id_mem_peak;       // 当前线程mem峰值
  unsigned int      current_id_mem_peak_time_s; // 峰值出现时间（秒）
  unsigned int      current_id_mem_peak_time_us; // 峰值出现时间（微妙）
} Tid;

Tid* g_thread_heap = NULL;
Tid* g_thread_tail = NULL;
static unsigned int g_mem_total = 0;
static unsigned int g_mem_head_total = 0;
static unsigned int g_tid_mem_toal = 0;
vzes::CriticalSection *g_md_mutex_;

// 互斥锁，确保锁先初始化
vzes::CriticalSection *GetCriticalInstance() {
  if (g_md_mutex_ == NULL) {
    static vzes::CriticalSection cs;
    g_md_mutex_ = &cs;
  }
  return g_md_mutex_;
}

/*
void BackTrace(MEM_INODE *pcur) {
  static HANDLE process = NULL;
  if (NULL == process) {
    // 获得进程句柄
    process = GetCurrentProcess();
    //
    SymInitialize(process, NULL, TRUE);
  }

  SYMBOL_INFO  *symbol = NULL;
  symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 128 * sizeof(char), 1);
  symbol->MaxNameLen = 127;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  void *stack[100] = { NULL };
  WORD frames = CaptureStackBackTrace(0, 100, stack, NULL);
  for (int i = 0; i < frames; i++) {
    SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
    // printf( "%s\n", symbol->Name );
    // printf("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
#if 0
    if (strstr(symbol->Name, "MemBuffer")) {
      i += 1;
      if (i < frames) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        pcur->func_line_ = frames - i - 1;
        strcpy(pcur->func_name_, symbol->Name);
        break;
      }
    }
#else
    // if (strstr(symbol->Name, "MemBuffer")) {
    if (i == 11) {
      SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
      pcur->func_line_ = frames - i - 1;
      strcpy(pcur->func_name_, symbol->Name);
      break;
    }
#endif
  }
  free(symbol);
}
*/

int PeakCompare(Tid *tid) {
  if (tid->current_id_mem_total > tid->current_id_mem_peak) {
    tid->current_id_mem_peak = tid->current_id_mem_total;
    tid->current_id_mem_peak_time_s = tid->lastmem->alloc_time_s;
    tid->current_id_mem_peak_time_us = tid->lastmem->alloc_time_us;
    return 0;
  }
  return -1;
}

unsigned int ThreadID() {
#ifndef _WIN32
  return pthread_self();
#elif WIN32
  return GetCurrentThreadId();
#endif
  return 0;
}

char* GetThreadName() {
  return const_cast<char*>(vzes::ThreadManager::Instance()->CurrentThread()->name().c_str());
}

void TimeSecond(MEM_INODE *pNode) {
#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  pNode->alloc_time_s = tv.sec;
  pNode->alloc_time_us = tv.usec + offset;
#else
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  pNode->alloc_time_s = tv.sec;
  pNode->alloc_time_us = tv.usec;
#endif
}

int TidInsert(MEM_INODE *tid_node) {
  if (!tid_node) {
    return -1;
  }
  // 如果是第一个线程
  if (!g_thread_heap) {
    g_thread_heap = (Tid *)malloc(sizeof(Tid));
    g_tid_mem_toal += sizeof(Tid);
    g_thread_heap->thread_id_ = tid_node->thread_id_;
    g_thread_heap->thread_name_[0] = '\0';
    g_thread_heap->current_id_mem_total = 0;
    g_thread_heap->current_id_mem_peak = 0;
    g_thread_heap->current_id_mem_peak_time_s = 0;
    g_thread_heap->current_id_mem_peak_time_us = 0;
    g_thread_heap->next_ = NULL;
    g_thread_tail = g_thread_heap;
  }
  Tid *tid = g_thread_heap;
  while (tid->thread_id_ != tid_node->thread_id_) {
    tid = tid->next_;
    // 如果是新线程
    if (!tid) {
      tid = (Tid *)malloc(sizeof(Tid));
      g_tid_mem_toal += sizeof(Tid);
      tid->thread_id_ = tid_node->thread_id_;
      tid->thread_name_[0] = '\0';
      tid->current_id_mem_total = 0;
      tid->current_id_mem_peak = 0;
      tid->current_id_mem_peak_time_s = 0;
      tid->current_id_mem_peak_time_us = 0;
      tid->next_ = NULL;
      g_thread_tail->next_ = tid;
      g_thread_tail = g_thread_tail->next_;
    }
  }
  // 如果是该线程上的第一个节点
  if (!tid->current_id_mem_total) {
    tid->fastmem = tid_node;
    tid->lastmem = tid_node;
    tid->current_id_max_mem_size = tid_node->mem_size_;
    strcpy(tid->max_mem_size_func_name, tid_node->func_name_);
    tid->max_mem_size_func_line_ = tid_node->func_line_;
    tid->current_id_mem_total += (tid_node->mem_size_ + sizeof(MEM_INODE));
    tid->current_id_mem_peak = tid->current_id_mem_total;
    tid->current_id_mem_peak_time_s = tid->lastmem->alloc_time_s;
    tid->current_id_mem_peak_time_us = tid->lastmem->alloc_time_us;
    return 0;
  }
  // 如果不是第一个节点
  tid->current_id_mem_total += (tid_node->mem_size_ + sizeof(MEM_INODE));
  //if (!strlen(tid->thread_name_)) {
  //  strcpy(tid->thread_name_,GetThreadName());
  //}
  PeakCompare(tid);
  if (tid->current_id_max_mem_size < tid_node->mem_size_) {
    tid->current_id_max_mem_size = tid_node->mem_size_;
    strcpy(tid->max_mem_size_func_name, tid_node->func_name_);
    tid->max_mem_size_func_line_ = tid_node->func_line_;
  }
  tid->lastmem->next_ = tid_node;
  tid->lastmem = tid->lastmem->next_;
  return 0;
}

int Insert(MEM_INODE *pNode, unsigned int nSize,
           const char *pFunc, unsigned int nLine) {
  if (!pNode) {
    return -1;
  }
  pNode->next_ = NULL;
  TimeSecond(pNode); // 分配时间
  pNode->thread_id_ = ThreadID(); // 所属线程
  pNode->mem_size_ = nSize;
  strcpy(pNode->func_name_,pFunc);
  pNode->func_line_ = nLine;
  //BackTrace(pNode);
  TidInsert(pNode); // 加入线程id链表
  g_mem_head_total = g_mem_head_total + sizeof(MEM_INODE);
  g_mem_total += (pNode->mem_size_ + sizeof(MEM_INODE));
  return 0;
}

MEM_INODE *TidDelete(unsigned long int thread_id, void *pPtr) {
  if (!g_thread_heap) {
    printf("Tid delete false,thread id is: %d\n",thread_id);
    return NULL;
  }
  // 从Tid链表的第一个节点开始
  Tid* tid = g_thread_heap, *pre_tid = g_thread_heap;
  while (tid->thread_id_ != thread_id) {
    pre_tid = tid;
    tid = tid->next_;
    // 如果没有该线程
    if (!tid) {
      printf("Tid delete false,thread id is: %d\n", thread_id);
      return NULL;
    }
  }
  // 如果该线程上没有节点,删掉
  if (tid->current_id_mem_total == 0) {
    // 如果删掉的线程链表的头节点
    if (tid == g_thread_heap) {
      g_thread_heap = g_thread_heap->next_;
    } else if (tid == g_thread_tail) {
      //  如果删掉的是线程链表的尾结点
      g_thread_tail = pre_tid;
      pre_tid->next_ = NULL;
    } else {
      // 如果删掉的是线程链表的中间节点
      pre_tid->next_ = tid->next_;
    }
    g_tid_mem_toal -= sizeof(Tid);
    free(tid);
    printf("Tid delete false, thread id is: %d\n", thread_id);
    return NULL;
  }
  MEM_INODE *pre_pcur = tid->fastmem, *pcur = tid->fastmem;
  while (pcur) {
    if (pcur->mem_addr_ == pPtr) {
      // 如果删除该线程的头结点
      if (pcur == tid->fastmem) {
        tid->fastmem = tid->fastmem->next_;
      } else if (pcur == tid->lastmem) {
        // 如果删除的是尾结点
        tid->lastmem = pre_pcur;
        pre_pcur->next_ = NULL;
      } else {
        // 如果是中间节点
        pre_pcur->next_ = pcur->next_;
        pcur->next_ = NULL;
      }
      tid->current_id_mem_total -= (pcur->mem_size_ + sizeof(MEM_INODE));
      // 如果删掉pcur之后线程没有内存管理，删掉
      if (tid->current_id_mem_total == 0) {
        // 如果删掉的线程链表的头节点
        if (tid == g_thread_heap) {
          g_thread_heap = g_thread_heap->next_;
        } else if (tid == g_thread_tail) {
          //  如果删掉的是线程链表的尾结点
          g_thread_tail = pre_tid;
          pre_tid->next_ = NULL;
        } else {
          // 如果删掉的是线程链表的中间节点
          pre_tid->next_ = tid->next_;
        }
        g_tid_mem_toal -= sizeof(Tid);
        free(tid);
      }
      return pcur;
    }
    pre_pcur = pcur;
    pcur = pcur->next_;
  }
  return NULL;
}

MEM_INODE *Delete(void *pPtr) {
  if (!pPtr) {
    return NULL;
  }
  unsigned long int thread_id;
  thread_id = (unsigned long int)*((int*)pPtr - 1);// 结构体虚拟地址是连在一起的
  MEM_INODE* pcur = TidDelete(thread_id, pPtr);
  if (pcur) {
    g_mem_head_total = g_mem_head_total - sizeof(MEM_INODE);
    g_mem_total -= (pcur->mem_size_ + sizeof(MEM_INODE));
    return pcur;
  }
  printf(">> %lu can't find this memory point 0x%x.\n", thread_id, pPtr);
  return NULL;
}

void DumpMemUseTotal() {
  printf("== memory use total %u.\n", g_mem_total);
}

void DumpMemUseToFile(const MEM_DUMP_TYPE_E usr_type,
                      const unsigned int thread_id) {
  vzes::CritScope cs(GetCriticalInstance());
  if (!g_thread_heap) {
    printf("No MemUse Info!\n");
    return;
  }
  FILE *file = fopen(FILE_PATH,"w");
  printf("[filepath]:%s\n",FILE_PATH);
  if (!file) {
    printf("[MEM_DUMP]:file open error!\n");
    return;
  }
  fprintf(file, "tid list count %d \nall thread count %d\n", g_tid_mem_toal, g_mem_total);
  if (usr_type & TOTAL) {
    fprintf(file,
            "thread_id\t thread_name\t mem_use\t peak_value\t peak_occurrence_time\t histor_max_mem\t file\t line\t\n\n");
    Tid *tid = g_thread_heap;
    if (thread_id != 0) {
      while (tid->thread_id_ != thread_id) {
        tid = tid->next_;
        if (!tid) {
          puts("[MEM_DUMP]:Search false!can't find this thread_id");
          fclose(file);
          return;
        }
      }
    }
    char buf[510];
    vzes::TimeLocal tm;
    while (tid) {
      vzes::TimeMkLocal(&tm, tid->current_id_mem_peak_time_s);
      sprintf(buf, "%u\t\t%s\t\t%u\t\t%u\t\t%02d:%02d:%02d:%02d\t%u\t%s\t%u\n", \
              tid->thread_id_, tid->thread_name_, tid->current_id_mem_total,
              tid->current_id_mem_peak, tm.hour, tm.min, tm.sec,
              tid->current_id_mem_peak_time_us, tid->current_id_max_mem_size,
              tid->max_mem_size_func_name, tid->max_mem_size_func_line_);
      fwrite(buf, strlen(buf), 1, file);
      if (thread_id == 0) {
        tid = tid->next_;
      }
    }
  }
  if (usr_type & DETAIL) {
    fprintf(file, "\nDETAIL:\n");
    Tid* tid = g_thread_heap;
    if (thread_id != 0) {
      while (tid->thread_id_ != thread_id) {
        tid = tid->next_;
        if (!tid) {
          puts("[MEM_DUMP]:learch false!can't find this thread_id");
          fclose(file);
          return;
        }
      }
    }
    while (tid) {
      MEM_INODE* pcur = tid->fastmem;
      if (tid->current_id_mem_total) {
        char memuse[500];
        vzes::TimeLocal tm;
        while (pcur) {
          vzes::TimeMkLocal(&tm, pcur->alloc_time_s);
          sprintf(memuse,
                  "[%4d-%02d-%02d %02d:%02d:%02d:%02d]:thread:%lu\tfile:%s\tline:%d\tmem_size:%d\n",
                  tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec,
                  pcur->alloc_time_us,
                  pcur->thread_id_, pcur->func_name_, pcur->func_line_,
                  pcur->mem_size_);
          fwrite(memuse, strlen(memuse), 1, file);
          pcur = pcur->next_;
        }
      }
      if (thread_id == 0) {
        tid = tid->next_;
      }
    }
  }
  fclose(file);
  printf("Mem use info to file done!\n");
}

void DumpMemUseToPoint(const MEM_DUMP_TYPE_E usr_type, const unsigned int thread_id) {
  vzes::CritScope cs(GetCriticalInstance());
  if (!g_thread_heap) {
    printf("No MemUse Info!\n");
    return;
  }
  if (usr_type & TOTAL) {
    puts("");
    printf("total thread header size: %d \n total memory header size: %d\n total memory size: %d\n",
           g_tid_mem_toal, g_mem_head_total, g_mem_total);
    puts("thread_id\\thread_name\\mem_use\\peak_value\\peak_occurrence_time\\histor_max_mem\\file\\line\n");
    Tid *tid = g_thread_heap;
    if (thread_id != 0) {
      while (tid->thread_id_ != thread_id) {
        tid = tid->next_;
        if (!tid) {
          puts("[MEM_DUMP]:learch false!can't find this thread_id");
          return;
        }
      }
    }
    char buf[510];
    vzes::TimeLocal tm;
    while (tid) {
      vzes::TimeMkLocal(&tm, tid->current_id_mem_peak_time_s);
      sprintf(buf, "%u\t%s\t%u\t%u\t%02d:%02d:%02d:%02d\t%u\t%s\t%u", \
              tid->thread_id_, tid->thread_name_, tid->current_id_mem_total,
              tid->current_id_mem_peak, tm.hour, tm.min, tm.sec,
              tid->current_id_mem_peak_time_us,tid->current_id_max_mem_size,
              tid->max_mem_size_func_name,tid->max_mem_size_func_line_);
      printf("%s\n", buf);
      if (thread_id == 0) {
        tid = tid->next_;
      }
    }
  }
  if (usr_type & DETAIL) {
    puts("\nDETAIL:\n");
    Tid* tid = g_thread_heap;
    if (thread_id != 0) {
      while (tid->thread_id_ != thread_id) {
        tid = tid->next_;
        if (!tid) {
          puts("[MEM_DUMP]:learch false!can't find this thread_id");
          return;
        }
      }
    }
    while (tid) {
      MEM_INODE* pcur = tid->fastmem;
      if (tid->current_id_mem_total) {
        char memuse[500];
        vzes::TimeLocal tm;
        while (pcur) {
          vzes::TimeMkLocal(&tm, pcur->alloc_time_s);
          sprintf(memuse,
                  "[%4d-%02d-%02d %02d:%02d:%02d:%02d]:thread:%lu\tfile:%s\tline:%d\tmem_size:%d\n",
                  tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec,
                  pcur->alloc_time_us,
                  pcur->thread_id_, pcur->func_name_, pcur->func_line_,
                  pcur->mem_size_);
          printf("%s",memuse);
          pcur = pcur->next_;
        }
      }
      if (thread_id == 0) {
        tid = tid->next_;
      }
    }
  }
  printf("Mem use info point done!\n");
}

void DumpMemInfo(const MEM_DUMP_END_E usr_end,
                 const MEM_DUMP_TYPE_E usr_type) {
  if (usr_end & (TO_POINT | TO_FILE)) {
    if (usr_type & (TOTAL | DETAIL)) {
      if (usr_end & TO_POINT) {
        DumpMemUseToPoint(usr_type, 0);
      }
      if (usr_end & TO_FILE) {
        DumpMemUseToFile(usr_type, 0);
      }
      return;
    }
    printf("[MEM_DUMP]:Undefined Type\n");
  }
  printf("[MEM_DUMP]:Undefined End\n");
}

extern void DumpSingleThreadMemInfo(const unsigned int thread_id,
                                    const MEM_DUMP_END_E usr_end,
                                    const MEM_DUMP_TYPE_E usr_type) {
  if (usr_end & (TO_POINT | TO_FILE)) {
    if (usr_type & (TOTAL | DETAIL)) {
      if (usr_end & TO_POINT) {
        DumpMemUseToPoint(usr_type, thread_id);
      }
      if (usr_end & TO_FILE) {
        DumpMemUseToFile(usr_type, thread_id);
      }
      return;
    }
    printf("[MEM_DUMP]:Undefined Type\n");
  }
  printf("[MEM_DUMP]:Undefined End\n");
}

char *GetFileName(char* file) {
  char *file_name;
#ifdef WIN32
  file_name = strrchr(file, '\\');
#else
  file_name = strrchr(file, '/');
#endif
  if (NULL == file_name) {
    file_name = file;
  } else {
    file_name += 1;
  }
  // 超长截断
  if (strlen(file_name) > 32) {
    char file_name_s[32];
    strncpy(file_name_s, file_name, 32);
    return file_name_s;
  }
  return file_name;
}

void* own_malloc(unsigned int nSize, char *pFile, unsigned int nLine) {
  vzes::CritScope cs(GetCriticalInstance());
  void *pUser = NULL;
  pUser = malloc(nSize + sizeof(MEM_INODE));
  if (!pUser) {
    printf("own_malloc: 0x%x %d.\n", ThreadID(), nSize);
    return NULL;
  }
  char *file = pFile;
  file = GetFileName(file);
  MEM_INODE *pNode = (MEM_INODE*)pUser;
  Insert(pNode, nSize, file, nLine);
  return (void*)pNode->mem_addr_;
}

void *own_realloc(void *pPtr, unsigned int nSize, char *pFile,
                  unsigned int nLine) {
  vzes::CritScope cs(GetCriticalInstance());

  void *pUser = NULL;
  MEM_INODE *pDelNode = Delete(pPtr);
  if (!pDelNode) {
    return own_malloc(nSize, pFile, nLine);
  }

  pUser = realloc(pDelNode, nSize + sizeof(MEM_INODE));
  if (!pUser) {
    printf("own_realloc: 0x%x %d.\n", ThreadID(), nSize);
    return ((void*)0);
  }
  MEM_INODE *pNode = (MEM_INODE*)pUser;
  Insert(pNode, nSize, pFile, nLine);
  return (void*)pNode->mem_addr_;
}

void own_free(void *pPtr, char *pFile, unsigned int nLine) {
  vzes::CritScope cs(GetCriticalInstance());
  if (!pPtr) {
    return;
  }

  MEM_INODE *pDelNode = Delete(pPtr);
  if (!pDelNode) {
    return;
  }
  free(pDelNode);
}

#if 1
void* operator new(unsigned int nSize, char *pFile, int nLine) _THROW {
  void *pUser = NULL;
  pUser = own_malloc(nSize, pFile, nLine);
  if (!pUser) {
    printf("new: thread %u size %d.\n", ThreadID(), nSize);
    return ((void*)0);
  }
  return pUser;
}

void* operator new[](unsigned int nSize, char *pFile, int nLine) _THROW {
  void *pUser = NULL;
  pUser = own_malloc(nSize, pFile, nLine);
  if (!pUser) {
    printf("new[]: thread %u size %d.\n", ThreadID(), nSize);
    return ((void*)0);
  }
  return pUser;
}

void operator delete(void* pPtr) _NOEXCEPT {
  if (!pPtr) {
    return;
  }
  VZ_FREE(pPtr);
  // printf(">>> delete %p.\n", pPtr);
}

void operator delete[](void* pPtr) _NOEXCEPT {
  if (!pPtr) {
    return;
  }
  VZ_FREE(pPtr);
  // printf(">>> delete[] %p.\n", pPtr);
}

void* operator new(unsigned int n) _THROW {
  return ::operator new(n, (char*)"(unknown)", 0);
}

void* operator new[](unsigned int n) _THROW {
  return ::operator new[](n, (char*)"(unknown)", 0);
}
#endif
#endif  // #ifdef MEM_TEST
