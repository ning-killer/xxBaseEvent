/*
* vzes
* Copyright 2013 - 2018, Vzenith Inc.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*  3. The name of the author may not be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#if defined(WIN32)
#include <comdef.h>
#include <windows.h>
#elif defined(POSIX)
#include <time.h>
#include <unistd.h> // syscall
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/syscall.h> // SYS_gettid
#endif
#include "eventservice/base/atomic.h"
#include "eventservice/base/common.h"
#include "eventservice/base/timeutils.h"
#include "eventservice/event/thread.h"
#include "log/log/log_server.h"
#include "log/log/log_client.h"
#include "log/log/log_cache.h"
#include "astl/mem_dump.h"


// 编译宏：格式化运行线程名称到Debug日志，liteos系统大概耗时10us，
// 其他系统开启，暂时关闭该功能
#if !defined(LITEOS)
#define LOG_FORMAT_THREAD_NAME
#endif

// 模块Debug日志配置信息
typedef struct {
  uint8  level;   // 日志级别
  char   name[LOG_MODULE_NAME_LEN]; // 模块名称
} LOG_DBG_MODULE_S;

// 日志模块实体
typedef struct {
  vzes::ThreadManager* thread_mgr;

  bool  sync_print_enable;          // 日志同步输出使能位
  LOG_PRINT_END_E  dbg_print_end;   // Debug日志输出终端设备
  volatile long    dbg_module_num;  // Debug日志模块数量
  LOG_DBG_MODULE_S dbg_module[LOG_MODULE_MAX_NUM]; // Debug日志模块配置信息
} LOG_CLIENT_ENTITY_S;


LOG_CLIENT_ENTITY_S *g_LogClient = NULL;

#ifdef LITEOS
#define LOG_CONTROL_THRESHOLD    (1200)  // 1616 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32,       64}, // 2K
  {64,      256}, // 16K
  {128,     768}, // 96K
  {256,     512}, // 128K
  {1*1024,    8}, // 8K
  {2*1024,    4}, // 8K
  {4*1024,    4}, // 16K,total 274K
};
#elif defined WIN32
#define LOG_CONTROL_THRESHOLD    (2200)  // 2928 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32,      256}, // 8K
  {64,      512}, // 32K
  {128,    4096}, // 512K
  {256,    4096}, // 1024k
  {1*1024,   64}, // 64K
  {2*1024,   32}, // 64K
  {4*1024,   16}, // 64K,total 1768k
};
#else // linux
#ifdef CACHE_SIZE_LOW
#define LOG_CONTROL_THRESHOLD    (1200)  // 1616 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32,       64}, // 2K
  {64,      256}, // 16K
  {128,     768}, // 96K
  {256,     512}, // 128K
  {1*1024,    8}, // 8K
  {2*1024,    4}, // 8K
  {4*1024,    4}, // 16K,total 274K
};
#else
#define LOG_CONTROL_THRESHOLD    (2000)  // 2656 * 0.8，Debug日志流控门限
LOG_CACHE_CFG_S log_cache_cfg[] = {
  {32,       64}, // 2K
  {64,      512}, // 32K
  {128,    1024}, // 128K
  {256,    1024}, // 256K
  {1*1024,   16}, // 16K
  {2*1024,    8}, // 16K
  {4*1024,    8}, // 32K,total 482K
};
#endif
#endif


// 原子添加LOG_NODE_S信息回调函数，添加时间信息
// pvMem:LOG_NODE_S指针
void AppendTime (void *pvMem) {
  LOG_NODE_S *cache = (LOG_NODE_S*)pvMem;
  if (NULL == cache) {
    return;
  }

#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  cache->sec = tv.sec;
  cache->usec = tv.usec + offset;
#else
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  cache->sec = tv.sec;
  cache->usec = tv.usec;
#endif
}

// 发送日志
// type:日志类型
// level:日志级别，Debug日志有效, LOG_LEVEL_E
// data:日志数据buffer
// size:数据长度
// return 成功:true；失败:false
bool WriteCache(int type, int level, char *data, int size) {
  int len = size + sizeof(LOG_NODE_S);
  LOG_NODE_S *cache = (LOG_NODE_S*)Log_CacheMalloc(len);
  if (NULL == cache) {
    //printf("[log]error:malloc cache buffer failed, size:%d\n", len);
    return false;
  }

  //memcpy(cache->body, data, size);
  memcpy((uint8 *)cache + sizeof(LOG_NODE_S), data, size);
  cache->type = type;
  cache->level = level;
  cache->length = len;
  if (LOG_ERROR == Log_CacheAdd(cache)) {
    (void)Log_CacheFree(cache);
    return false;
  }
  // printf("WriteData(%s).\n", data);
  // char data1[LOG_DEBUG_MAX_LEN] = { 0 };
  // memcpy(data1, (uint8 *)cache + sizeof(LOG_NODE_S), cache->length - sizeof(LOG_NODE_S));
  // printf("WriteCache(%s).\n", (char*)data1);
  return true;
}

void SyncPrintNode(LOG_NODE_S *node,char *time_s) {
#if defined(WIN32)
#define LPN_COLOR_RED (FOREGROUND_RED)
#define LPN_COLOR_GREEN (FOREGROUND_GREEN)
#define LPN_COLOR_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define LPN_COLOR_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

  switch (node->level) {
  case LL_ERROR:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_RED);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_WARNING:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_YELLOW);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_KEY:
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_GREEN);
    printf("%s\n", time_s);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
    break;
  case LL_NONE:
  default:
    printf("%s\n", time_s);
    break;
  }
#else
  switch (node->level) {
  case LL_ERROR:
    printf("\033[1;31;40m%s\033[0m\n", time_s);
    break;
  case LL_WARNING:
    printf("\033[1;33;40m%s\033[0m\n", time_s);
    break;
  case LL_KEY:
    printf("\033[1;32;40m%s\033[0m\n", time_s);
    break;
  case LL_NONE:
  default:
    printf("%s\n", time_s);
    break;
  }
#endif
}

void SyncReadyOutputString(LOG_NODE_S *node, char *data) {
  char time_s[1020];
  vzes::TimeLocal tm;
  vzes::TimeMkLocal(&tm, node->sec);
  const char *kLogLevelString[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "KEY", "-"
  };

  switch (node->level) {
  case LL_INFO:
  case LL_ERROR:
  case LL_WARNING:
  case LL_KEY:
  case LL_DEBUG:
    snprintf(time_s, 1020\
             , "%04d-%02d-%02d %02d:%02d:%02d,%-6d,[%s]: %s"\
             , tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec\
             , kLogLevelString[node->level], data);
    break;
  case LL_NONE:
    snprintf(time_s, 1020\
             , "%04d-%02d-%02d %02d:%02d:%02d,%-6d: %s"\
             , tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec\
             , data);
    break;
  }
  SyncPrintNode(node, time_s);
}

// DEBUG日志Client端打印
void SyncPrint(int type, int level, char *data, int size) {
  //将日志数据拼成一个字符串，然后输出
  // 建一个LOG信息节点头
  LOG_NODE_S cache[sizeof(LOG_NODE_S)];
  int len = size + sizeof(LOG_NODE_S);
  AppendTime(cache);
  cache->type = type;
  cache->level = level;
  cache->length = len;
  SyncReadyOutputString(cache,data);
}

// Debug日志流控开启状态检查
// level: 日志级别,LOG_LEVEL_E
// return 开启:true；未开启:false
bool TraficControl(int level) {
  int log_count = Log_CacheCnt();
  if ((LOG_CONTROL_THRESHOLD <= log_count) && (LL_WARNING > level)) {
    return true;
  }
  return false;
}

// 从文件全路径__FILE__截取文件名
// file: 文件全路径
inline char *CatFileName(char* file) {
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

  return file_name;
}

// 基础库MID注册，默认为第一个ID
void EventBaseReg(LOG_CLIENT_ENTITY_S *log_client) {
  //EventBase use the first ID: MOD_EB
  log_client->dbg_module[0].level = LL_INFO;
  snprintf(log_client->dbg_module[0].name, LOG_MODULE_NAME_LEN, "event_base");
  log_client->dbg_module_num ++;
}

bool InitService(bool multi_process, bool cache_exist) {
  LOG_SRV_CONFIG_S config;
  Log_SrvConfigGet(&config);
  bool ret = Log_SrvInit(&config);
  if (!ret) {
    printf("[log]error:log service init failed\n");
    return false;
  }

  if (cache_exist) {
    ret = Log_CacheReset();
  } else {
    ret = Log_CacheCreate(log_cache_cfg,
                          sizeof(log_cache_cfg)/sizeof(LOG_CACHE_CFG_S),
                          multi_process);
  }
  if (!ret) {
    printf("[log]error:create log cache failed\n");
    return false;
  }
  return true;
}

void Log_HexDump(const char *discript, const unsigned char *data,
                 uint32 size) {
  if ((NULL == data) || (0 == size)) {
    return;
  }
  if (discript) {
    printf("%s,", discript);
  }
  printf("data: ");
  for(uint32 i = 0; i < size; i++) {
    printf("%X ", (int)(data[i]));
  }
  printf("\n");
}

void Log_RelTraceMap2(int type, uint16 id_1, uint16 id_2,
                      char *format, ...) {
  if (!g_LogClient) {
    return;
  }

  char data[LOG_RELEASE_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_RELEASE_MAX_LEN - sizeof(LOG_NODE_S) - 1;
  int size = snprintf(data, limit, "%d,%d,", id_1, id_2);

  if (format) {
    va_list args;
    va_start(args, format);
    limit = limit - size;
    // Linux系统如果format长度超过源buffer的长度，则截断format，并返回
    // format的实际长度，字符串转换错误返回-1；Windows系统如果format长
    // 度超过源buffer的长度，则截断format，返回-1
    int ret = vsnprintf(data + size, limit, format, args);
    va_end(args);
    if (ret != -1) {
      size += VZ_MIN(ret, limit);
    } else {
#ifdef WIN32
      size += limit;
#endif
    }
  }
  data[size] = '\0';
  size += 1;
  (void)WriteCache(type, LL_NONE, data, size);
}

bool Log_SetSyncPrint(bool on) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited，SetSyncPrint failed\n");
    return false;
  }
  if (on == g_LogClient->sync_print_enable) {
    return true;
  }
  g_LogClient->sync_print_enable = on;
  printf("[logClient]the sync_print_enable is :%d\n",
         g_LogClient->sync_print_enable);
  return true;
}

void Log_RelTrace(int type, char *format, ...) {
  if (!g_LogClient) {
    return;
  }

  int size = 0;
  char data[LOG_RELEASE_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_RELEASE_MAX_LEN - sizeof(LOG_NODE_S) - 1;
  if (format) {
    va_list args;
    va_start(args, format);
    limit = limit - size;
    int ret = vsnprintf(data + size, limit, format, args);
    va_end(args);
    if (ret != -1) {
      size += VZ_MIN(ret, limit);
    } else {
#ifdef WIN32
      size += limit;
#endif
    }
  }
  data[size] = '\0';
  size += 1;
  if (g_LogClient->sync_print_enable) {
    LOG_SRV_FILE_TYPE_E fid;
    Log_SrvGetFileType(type, &fid);
    if (LogPrintEnd[fid] & LE_LOCAL) {
      SyncPrint(type, LL_NONE, data, size);
      return;
    }
  }
  (void)WriteCache(type, LL_NONE, data, size);
}

void Log_DbgTrace(int mod_id, int level, char* file,
                  uint32 line, char *format, ...) {
  if (!g_LogClient) {
    return;
  }
  uint32 pos = mod_id - LOG_MODULE_ID_BASE;
  if (pos >= (uint32)g_LogClient->dbg_module_num) {
    printf("[log]error:DbgTrace failed, invalid mid:%d\n", mod_id);
    return;
  }
  if ((level < g_LogClient->dbg_module[pos].level)
      || (level >= LL_NONE)
      || (LE_OFF == g_LogClient->dbg_print_end)) {
    return;
  }

  // 流量控制
  if (TraficControl(level)) {
    static uint32 count = 0;
    if (0 == (count % 64)) {
      Log_DumpChacheInfo();
    }
    count ++;
    return;
  }

  int size = 0, offset = 0;
  char data[LOG_DEBUG_MAX_LEN + 1];
  // 可用长度，预留LOG_NODE_S 和 "\0"空间
  int limit = LOG_DEBUG_MAX_LEN - sizeof(LOG_NODE_S) - 1;
#ifdef LOG_FORMAT_THREAD_NAME
  // 格式化运行线程名称
  bool known_name = false;
  if (g_LogClient->thread_mgr) {
    vzes::Thread *thread = g_LogClient->thread_mgr->CurrentThread();
    if (thread) {
      const char *thread_name = thread->name().c_str();
      size = snprintf(data + offset, limit, "%s,", thread_name);
      limit -= size;
      offset += size;
      known_name = true;
    }
  }
  if (!known_name) {
#ifdef POSIX
    size = snprintf(data + offset, limit, "tid:%lu,", syscall(SYS_gettid));
#else
    size = snprintf(data + offset, limit, "%s,", "unnamed");
#endif
    limit -= size;
    offset += size;
  }
#endif

  char *file_name = CatFileName(file);
  size = snprintf(data + offset, limit, "%s,%d,", file_name, line);
  limit -= size;
  offset += size;
  if (format) {
    va_list args;
    va_start(args, format);
    size = vsnprintf(data + offset, limit, format, args);
    va_end(args);
    if (size != -1) {
      offset += VZ_MIN(size, limit);
    } else {
#ifdef WIN32
      offset += limit;
#endif
    }
  }

  data[offset] = '\0';
  offset += 1;
  //如果开启同步打印
  if (g_LogClient->sync_print_enable) {
    if (g_LogClient->dbg_print_end & LE_LOCAL) {
      SyncPrint(LT_DEBUG, level, data, offset);
      return;
    }
  }
  (void)WriteCache(LT_DEBUG, level, data, offset);
}

bool Log_DbgSetLevel(int mod_id, int level) {
  if (!g_LogClient) {
    printf("[log]error:set log level failed(mid:%d, leve:%d)\n", mod_id, level);
    return false;
  }

  uint32 pos = mod_id - LOG_MODULE_ID_BASE;
  if (pos >= LOG_MODULE_MAX_NUM) {
    printf("[log]error:set log level failed, invalid mid:%d\n", mod_id);
    return false;
  }

  g_LogClient->dbg_module[pos].level = level;
  return true;
}

int Log_DbgModRegist(const char *name, int level) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited\n");
    return -1;
  }

  if (!name || (0 == strlen(name))) {
    printf("[log]error:invalid module name\n");
    return -1;
  }

  do {
    // 查找是否已注册模块
    int pos = -1;
    for (int i=0; i<g_LogClient->dbg_module_num; i++) {
      if (0 == strcmp(name, g_LogClient->dbg_module[i].name)) {
        pos = i;
        break;
      }
    }

    if (-1 == pos) {
      uint32 curr_num = VZ_FAA(&(g_LogClient->dbg_module_num), 1);
      if ((curr_num + 1) > LOG_MODULE_MAX_NUM) {
        (void)VZ_FAS(&(g_LogClient->dbg_module_num), 1);
        printf("[log]error:modules reach max num:%d\n", LOG_MODULE_MAX_NUM);
        break;
      }
      pos = curr_num;
      snprintf(g_LogClient->dbg_module[pos].name, LOG_MODULE_NAME_LEN, name);
    }

    g_LogClient->dbg_module[pos].level = level;
    return (pos + LOG_MODULE_ID_BASE);
  } while (0);

  printf("[log]error:module %s register failed\n", name);
  return -1;
}

bool Log_SetPrintEnd(int type, int print_end) {
  if (!g_LogClient) {
    printf("[log]error:log module uninited, Log_SetPrintEnd failed\n");
    return false;
  }

  if (LT_DEBUG == type) {
    g_LogClient->dbg_print_end = print_end;
  }
  return Log_SrvSetPrintEnd(type, print_end);
}

void Log_DumpChacheInfo() {
#define RES_ARRAY_SIZE  (16)
  uint8 cache_count = RES_ARRAY_SIZE;
  uint8 result = 0;
  uint32 total_size = 0;
  LOG_CACHE_INFO_S cache_info[RES_ARRAY_SIZE] = {0};
  result = Log_CacheInfo(cache_info, &cache_count);
  if (result > 0) {
    printf("log module level 1 cache usage:\n");
    printf("> stack | block size | block count | use count | max use count\n");
    for (int i=0; i<result; i++) {
      printf("> %-2d | %-5d | %-5d | %-5d | %-5d \n",
             i, cache_info[i].usBlockSize, cache_info[i].usBlockCnt,
             cache_info[i].usBlockCntUsed, cache_info[i].usBlockCntUsedMax);
      total_size  += cache_info[i].usBlockSize * cache_info[i].usBlockCnt;
    }
    printf("> total cache size %d(Byte)\n", total_size);
  }
}

bool Log_ClientInit(void) {
#if defined(WIN32) || defined(LITEOS)
  return false;
#endif

  if (g_LogClient) {
    printf("[log]log client already inited\n");
    return true;
  }
  LOG_CLIENT_ENTITY_S *log_client_entity = NULL;

  do {
    int size = sizeof(LOG_CLIENT_ENTITY_S);
    log_client_entity = (LOG_CLIENT_ENTITY_S*)VZ_MALLOC(size);
    if (NULL == log_client_entity) {
      printf("[log]error:malloc log entity failed, size:%d\n", size);
      break;
    }
    memset(log_client_entity, 0x00, size);
    log_client_entity->sync_print_enable = false;
    log_client_entity->dbg_print_end = LE_LOCAL;
    EventBaseReg(log_client_entity);
    int res = Log_CacheCheck();
    if (0 != res) {
      printf("[log]error:not found log server process\n");
      break;
    }

    printf("[log]:log client init successed\n");
    g_LogClient = log_client_entity;
    return true;
  } while (0);

  printf("[log]error:log client init failed\n");
  Log_Deinit();
  return false;
}


void Log_Deinit(void) {
  if (g_LogClient) {
    printf("[log]deinit log module\n");
    Log_CacheDestory();
    Log_SrvDeinit();
    VZ_FREE(g_LogClient);
    g_LogClient = NULL;
  }
}

bool Log_Init(bool multi_process) {
  if (g_LogClient) {
    printf("[log]log module already inited\n");
    return true;
  }
  LOG_CLIENT_ENTITY_S *log_client_entity = NULL;
  // Windows暂不支持Server、Client模式，每个进程独立处理日志
#if defined(WIN32) || defined(LITEOS)
  multi_process = false;
#endif

  do {
    int size = sizeof(LOG_CLIENT_ENTITY_S);
    log_client_entity = (LOG_CLIENT_ENTITY_S*)VZ_MALLOC(size);
    if (NULL == log_client_entity) {
      printf("[log]error:malloc log entity failed, size:%d\n", size);
      break;
    }
    memset(log_client_entity, 0x00, size);
    log_client_entity->sync_print_enable = false;
    log_client_entity->dbg_print_end = LE_LOCAL;
    EventBaseReg(log_client_entity);

    bool result = true;
    if (multi_process) {
      int ret = Log_CacheCheck();
      if (-1 == ret) {
        result = InitService(true, false);
      } else if (-2 == ret) {
        result = InitService(true, true);
      }
    } else {
      result = InitService(false, false);
    }

    if (!result) {
      break;
    }
#ifdef LOG_FORMAT_THREAD_NAME
    log_client_entity->thread_mgr = vzes::ThreadManager::Instance();
    if (NULL == log_client_entity->thread_mgr) {
      printf("[log]warning:get thread manager failed\n");
    }
#endif
    printf("[log]:log module init successed\n");
    g_LogClient = log_client_entity;
    return true;
  } while (0);

  printf("[log]error:log module init failed\n");
  Log_Deinit();
  return false;
}

