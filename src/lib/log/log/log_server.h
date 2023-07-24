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
#ifndef __LOG_SERVER_C_H__
#define __LOG_SERVER_C_H__

#include "eventservice/base/basictypes.h"
#include "log_client.h"
#if defined(WIN32)
#include <comdef.h>
#elif defined(POSIX)
#include <pthread.h>
#include <ostream>
#include <fstream>
#endif
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

// Log服务线程优先级定义
#if defined(WIN32)
#define  LOG_SERVER_PRIORITY    (THREAD_PRIORITY_IDLE)
#define LOGSVR_MUTEX_T          CRITICAL_SECTION
#define LOGSVR_MUTEX_INIT(X)    InitializeCriticalSection(&X)
#define LOGSVR_MUTEX_LOCK(X)    EnterCriticalSection(&X)
#define LOGSVR_MUTEX_UNLOCK(X)  LeaveCriticalSection(&X)
#define LOGSVR_MUTEX_DESTORY    DeleteCriticalSection(&X)

#define kLogFoldPathDefault     "c:\\vzlog"

#elif defined(POSIX)
#define LOG_SERVER_PRIORITY     (15)
#define LOGSVR_MUTEX_T          pthread_mutex_t
#define LOGSVR_MUTEX_INIT(X)    pthread_mutex_init(&X, NULL)
#define LOGSVR_MUTEX_LOCK(X)    pthread_mutex_lock(&X)
#define LOGSVR_MUTEX_UNLOCK(X)  pthread_mutex_unlock(&X)
#define LOGSVR_MUTEX_DESTORY(X) pthread_mutex_destory(&X)

#if defined(UBUNTU64)
#define kLogFoldPathDefault     "/tmp/vz/mnt/usr/vzlog"
#elif defined(LITEOS)
#define kLogFoldPathDefault     "/log/system_server_files"
#else
#define kLogFoldPathDefault     "/mnt/log/system_server_files"
#endif
#endif
// #define  LOG_SERVER_PERIODIC   (1000) // 服务线程睡眠时间(ms)

extern LOGSVR_MUTEX_T  g_log_srv_mutex;
#define LOGSVR_FILE_LOCK        LOGSVR_MUTEX_LOCK(g_log_srv_mutex)
#define LOGSVR_FILE_UNLOCK      LOGSVR_MUTEX_UNLOCK(g_log_srv_mutex)

#define LOG_FOlDPATH_MAXLEN     (128)    // 文件夹路径长度上限
#define LOG_FILFPATH_MAXLEN     (LOG_FOlDPATH_MAXLEN+24)  // 文件路径长度上限
#define LOG_FILF_GROUP_MAXCNT   (2)      // 子文件个数
#define LOG_FILE_NODE_HEAD      (0x47)   // 文件节点头标识

// 日志文件类型
typedef enum {
  LFT_DEBUG = 0,
  LFT_POINT,
  LFT_SYS,
  LFT_DEV,
  LFT_UI,
  LOG_SRV_FILE_TYPE_MAX
} LOG_SRV_FILE_TYPE_E;

// 日志服务器配置信息结构定义
typedef struct {
  uint32 ip;
  uint16 port;
  LOG_PRINT_END_E print_end[LOG_SRV_FILE_TYPE_MAX]; // 输出方式
  char foldpath[LOG_FOlDPATH_MAXLEN];               // 日志文件夹路径
  uint32 file_maxlen[LOG_SRV_FILE_TYPE_MAX];        // 文件上限(Byte)
  uint32 cache_maxsize[LOG_SRV_FILE_TYPE_MAX];      // 缓存上限(Byte)
  uint16 pull_wait_ms;                              // 写日志文件轮询等待时间(秒)
  uint16 write_overtime_cnt[LOG_SRV_FILE_TYPE_MAX]; // 写日志文件等待超时,n次等待时间
} LOG_SRV_CONFIG_S;

// 获取server端的日志输出终端数组
extern LOG_PRINT_END_E *LogPrintEnd;

// 日志模块初始化，基础库内部调用
// return 成功:true; 失败:false
bool Log_SrvInit(LOG_SRV_CONFIG_S *config);

// 日志模块去初始化，基础库内部调用
void Log_SrvDeinit(void);

// 获取日志模块配置认值默，基础库内部调用
// config 输出认默配置信息
void Log_SrvConfigDefaultGet(LOG_SRV_CONFIG_S *config);

// 获取日志模块配置，基础库内部调用
// config 出当前输配置信息
void Log_SrvConfigGet(LOG_SRV_CONFIG_S *config);

// 设置日志模块配置，基础库内部调用
// config 配置信息
bool Log_SrvConfigSet(LOG_SRV_CONFIG_S *config);

bool Log_SrvSetPrintEnd(LOG_TYPE_E type, LOG_PRINT_END_E print_end);

bool Log_SrvGetFileType(LOG_TYPE_E type, LOG_SRV_FILE_TYPE_E *out_fid);

bool Log_SrvGetType(LOG_SRV_FILE_TYPE_E fid, LOG_TYPE_E *out_type);

void Log_SrvGetFileName(LOG_SRV_FILE_TYPE_E fid, int index,
                        char *out_data_name, char *out_index_name, int maxlen);

void Log_SrvGetFileNameCurrent(LOG_SRV_FILE_TYPE_E fid,
                               char *out_data_name, char *out_index_name, int maxlen);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __LOG_SERVER_C_H__
