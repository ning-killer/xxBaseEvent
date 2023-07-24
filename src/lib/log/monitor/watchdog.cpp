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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#if defined(WIN32)
#include <windows.h>
#elif defined(POSIX)
#include <sys/prctl.h>
#endif

#include "eventservice/base/basictypes.h"
#include "eventservice/base/common.h"
#include "eventservice/base/atomic.h"
#include "log/monitor/watchdog.h"
#include "log/log/log_client.h"
#include "astl/mem_dump.h"

// WatchDog服务线程优先级定义
#if defined(WIN32)
#define WDG_SERVER_PRIORITY       (THREAD_PRIORITY_ABOVE_NORMAL)
#elif defined(POSIX)
#define WDG_SERVER_PRIORITY       (9)
#endif

#define WDG_MODULE_NAME_MAX_LEN   (32)		// 模块注册的最大名称长度
#define WDG_MODULE_KEY_BASE       (1001)	// 模块Key的基础值
#define WDG_MODULE_MAX_NUM        (64)		// 最大可注册的模块数量
#define WDG_THREAD_PERIODIC       (1000)	// 守护线程睡眠时间(ms)
#define WDG_SYS_DOG_PERIODIC      (30)		// 系统软狗超时时间
#define WDG_SYS_DOG_COUNT         (10)		// 喂系统软狗时间计数


typedef struct {
  char    name[WDG_MODULE_NAME_MAX_LEN];	// 模块名称
  bool    registered;						// 注册标识
  uint32  time_out;							// 超时时间
  volatile uint32  last_feed_time;			// 最后一次喂狗时间
} WatchDog_Module;

typedef struct {
  // 线程ID
#ifdef POSIX
  pthread_t  thread_;
#elif defined WIN32
  HANDLE     thread_;
  DWORD      thread_id_;
#endif
  int        sys_dog_cnt;     // 喂系统软狗时间技术
  bool       sys_dog_init;    // 系统软狗初始化标识
  bool       stop_flag;       // 看门狗模块退出标识
  bool       enable;          // 看门狗模块开关
  bool       cfgfile_loaded;  // 配置文件加载标识
  bool       reload_cfgfile;  // 重新加载配置文件标识
  volatile long module_num;   // 已注册模块数量
  preRebootCb reboot_cb;      // 用户添加看门狗回调函数
  uint32      reboot_cb_time; // 用户添加看门狗回调函数超时时间
  WatchDog_Module modules[WDG_MODULE_MAX_NUM];
} WatchDog_Entity;

WatchDog_Entity *g_WdgEntity = NULL;

#if !defined(WIN32) && !defined(UBUNTU64)
#if __cplusplus
extern "C" {
#endif
  extern int VzDeviceSDK_Init(int SDK_Version);
  extern int VZ_DeviceSDK_Watchdog_init(int);
  extern int VZ_DeviceSDK_Watchdog_SetTimeOut(int);
  extern int VZ_DeviceSDK_Watchdog_Feed(void);
  extern int VZ_DeviceSDK_Watchdog_Release(void);
  extern int VZ_DeviceSDK_Sys_Reboot(void);
  // LiteOS系统打印task信息
  // This is a GNU compiler extension that the ARM compiler supports.
  static uint32 VZ_osShellCmdTskInfoGetRef(uint32 uwTaskID, uint32 TransId) __attribute__((weakref("osShellCmdTskInfoGet")));
#if __cplusplus
}
#endif
#endif


unsigned int Wdg_GetSysSec() {
#ifdef WIN32
  return (unsigned int)(GetTickCount() / 1000);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (unsigned int)ts.tv_sec;
#endif
  return 0;
}

/**
 * 加载配置文件中定义的默认模块，这些模块需要在默认的超时时间
 * 内启动完成，并注册、喂狗，否则就会认为模块启动失败，重启系统
 */
bool Wdg_LoadDefModule(void) {
  if (g_WdgEntity->cfgfile_loaded) {
    DLOG_INFO(MOD_EB, "Default config file %s already loaded",
              WDG_DEF_MODULE_FILE);
    return true;
  }
  FILE* file = fopen(WDG_DEF_MODULE_FILE, "rt");
  if (NULL == file) {
    DLOG_ERROR(MOD_EB, "open file %s failed, errno:%s",
               WDG_DEF_MODULE_FILE, strerror(errno));
    return false;
  }

  WatchDog_Module *p_modules = g_WdgEntity->modules;
  uint32 curr_num = 0;

  do {
    char s_line[128 + 1] = {0};
    int  n_line = 0;

    //memset(s_line, 0, 128);
    fgets(s_line, 127, file);
    if ((n_line = strlen(s_line)) <= 0) {
      break;
    }
    if (s_line[0] == '#') {
      continue;
    }

    // 提取模块名称
    char s_app_name[WDG_MODULE_NAME_MAX_LEN + 1] = {0};
    int name_len  = 0;
    int n_timeout = 0;
    const char *p_app = strchr(s_line, ';');
    if (p_app == NULL) {
      DLOG_WARNING(MOD_EB, "invalid line");
      continue;
    }

    name_len = p_app - s_line;
    if (name_len > WDG_MODULE_NAME_MAX_LEN) {
      name_len = WDG_MODULE_NAME_MAX_LEN;
    } else if (0 == name_len) {
      DLOG_WARNING(MOD_EB, "invalid module name len: 0");
      continue;
    }

    // 过滤重复模块
    bool repeated = false;
    for (uint32 i=0; i<g_WdgEntity->module_num; i++) {
      if (0 == strncmp(p_modules[i].name, s_line, name_len)) {
        DLOG_WARNING(MOD_EB, "repeated module name %s", p_modules[i].name);
        repeated = true;
        break;
      }
    }

    if (repeated) {
      continue;
    } else {
      curr_num = VZ_FAA(&(g_WdgEntity->module_num), 1);
      if ((curr_num + 1) > WDG_MODULE_MAX_NUM) {
        (void)VZ_FAS(&(g_WdgEntity->module_num), 1);
        DLOG_ERROR(MOD_EB, "modules reach max num:%d", WDG_MODULE_MAX_NUM);
        break;
      }
      strncpy(p_modules[curr_num].name, s_line, name_len);
    }

    // 提取模块超时时间
    n_timeout = atoi(p_app + 1);
    if (n_timeout > WDG_MAX_TIMEOUT) {
      n_timeout = WDG_MAX_TIMEOUT;
      DLOG_WARNING(MOD_EB, "invalid time %d, resized to %d",
                   n_timeout, WDG_MAX_TIMEOUT);
    } else if (n_timeout < WDG_MIN_TIMEOUT) {
      n_timeout = WDG_MIN_TIMEOUT;
      DLOG_WARNING(MOD_EB, "invalid time %d, resized to %d",
                   n_timeout, WDG_MIN_TIMEOUT);
    }

    p_modules[curr_num].time_out = n_timeout;
    p_modules[curr_num].registered = true;
    p_modules[curr_num].last_feed_time = 0;
  } while ((!feof(file)) && (g_WdgEntity->module_num < WDG_MODULE_MAX_NUM));

  fclose(file);
  g_WdgEntity->cfgfile_loaded = true;
  DLOG_INFO(MOD_EB, "load defult cfg successed, total %d modules",
            g_WdgEntity->module_num);
  return true;
}

bool Wdg_InitSysWdg(void) {
#if !defined(WIN32) && !defined(UBUNTU64)
  /**
   * 系统启动后，驱动初始化系统软狗并设置超时时间为60s，应用需要在60s内
   * 启动完成并喂狗，否则软狗就会超时重启，应用启动后需要接管系统软狗，
   * 修改超时时间为30秒，并定时喂狗。
   */
  int retval = -1;
  retval = VzDeviceSDK_Init(0);
  if (retval < 0) {
    DLOG_ERROR(MOD_EB, "VzDeviceSDK_Init failed");
    return false;
  }

  retval = VZ_DeviceSDK_Watchdog_init(0);
  if (retval < 0) {
    DLOG_ERROR(MOD_EB, "VZ_DeviceSDK_Watchdog_init failed");
    return false;
  }

  retval = VZ_DeviceSDK_Watchdog_Feed();
  if (retval < 0) {
    DLOG_ERROR(MOD_EB, "VZ_DeviceSDK_Watchdog_Feed failed!");
  }

  retval = VZ_DeviceSDK_Watchdog_SetTimeOut(WDG_SYS_DOG_PERIODIC);
  if (retval < 0) {
    DLOG_ERROR(MOD_EB, "VZ_DeviceSDK_Watchdog_SetTimeOut failed");
  }
#endif
  return true;
}

int Wdg_FeedSysWdg(void) {
  int ret = 0;
  g_WdgEntity->sys_dog_cnt++;
#if !defined(WIN32) && !defined(UBUNTU64)
  if ((g_WdgEntity->sys_dog_init)
      && (WDG_SYS_DOG_COUNT == g_WdgEntity->sys_dog_cnt)) {
    g_WdgEntity->sys_dog_cnt = 0;
    ret = VZ_DeviceSDK_Watchdog_Feed();
    if (ret < 0) {
      DLOG_ERROR(MOD_EB, "feed system soft dog failed!");
    } else {
      DLOG_INFO(MOD_EB, "feed system soft dog successed");
    }
  }
#endif

  return ret;
}

void Wdg_DumpSystemInfo(void) {
#if !defined(WIN32) && !defined(UBUNTU64)
  if (VZ_osShellCmdTskInfoGetRef) {
    (void)VZ_osShellCmdTskInfoGetRef(0xffffffff, 0);
  }
#else
  // ...
#endif
}

bool Wdg_ThreadConfig(void) {
#if defined(WIN32)
  // 设置线程名称
  // As seen on MSDN.
  // http://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.71).aspx
#define MSDEV_SET_THREAD_NAME  0x406D1388
  typedef struct tagTHREADNAME_INFO {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  } THREADNAME_INFO;

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = "vz_watchDog";
  info.dwThreadID = g_WdgEntity->thread_id_;
  info.dwFlags = 0;

  __try {
    RaiseException(MSDEV_SET_THREAD_NAME, 0, sizeof(info) / sizeof(DWORD),
                   reinterpret_cast<ULONG_PTR*>(&info));
  } __except(EXCEPTION_CONTINUE_EXECUTION) {
  }

  //设置优先级
  ::SetThreadPriority(g_WdgEntity->thread_, WDG_SERVER_PRIORITY);
  return true;
#elif defined(POSIX)
  // 设置线程名称
  prctl(PR_SET_NAME, "vz_watchDog");

  // 设置线程优先级
  struct sched_param param;
  param.sched_priority = WDG_SERVER_PRIORITY;
  if (pthread_setschedparam(g_WdgEntity->thread_, SCHED_RR, &param)) {
    DLOG_ERROR(MOD_EB, "set log server priority failed");
    return false;
  }
  return true;
#endif
}

void* Wdg_NotifyThreadProc(void *data) {
  DLOG_INFO(MOD_EB, "Pre reboot notify user");
  g_WdgEntity->reboot_cb();
  DLOG_INFO(MOD_EB, "Pre reboot thread exit");
  return NULL;
}

int Wdg_PreRebootNotify() {
  if (!g_WdgEntity->reboot_cb) {
    return 0;
  }

#if defined(WIN32)
  HANDLE thread = NULL;
  thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Wdg_NotifyThreadProc,
                        NULL, 0, NULL);
  if (!thread) {
    DLOG_WARNING(MOD_EB, "create notify thread failed");
    return -1;
  }

  vzsleep(g_WdgEntity->reboot_cb_time);
  //CloseHandle(thread);
#elif defined(POSIX)
  int ret = 1;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  ret = pthread_create(&thread, &attr, Wdg_NotifyThreadProc, NULL);
  if (0 != ret) {
    DLOG_WARNING(MOD_EB, "create notify thread failed");
    return -1;
  }

  vzsleep(g_WdgEntity->reboot_cb_time);
  //pthread_cancel(thread);
#endif
  return 0;
}

void Wdg_TimeoutHdlr(void) {
  // 先记录日志，并刷新日志到文件中，然后重启系统
#define  LOG_MAX_LEN   (2048) // 确保足够长，不会溢出
  int  size_nor   = 0, size_out   = 0;
  int  offset_nor = 0, offset_out = 0;
  int  limit_nor  = LOG_MAX_LEN-1, limit_out = LOG_MAX_LEN-1;
  char data_nor[LOG_MAX_LEN] = {0};
  char data_out[LOG_MAX_LEN] = {0};

  size_out = snprintf(data_out, limit_out, "wdg timeout mod:");
  limit_out -= size_out;
  offset_out += size_out;
  size_nor = snprintf(data_nor, limit_nor, "wdg normal mod:");
  limit_nor -= size_nor;
  offset_nor += size_nor;
  DLOG_KEY(MOD_EB, "Watch dog timeout, will reboot system now");

  for (long i=0; i<g_WdgEntity->module_num; i++) {
    if (g_WdgEntity->modules[i].last_feed_time >
        g_WdgEntity->modules[i].time_out) {
      size_out = snprintf(data_out + offset_out, limit_out, "%.*s[%d,%d],",
                          5, g_WdgEntity->modules[i].name,
                          g_WdgEntity->modules[i].time_out,
                          g_WdgEntity->modules[i].last_feed_time);
      limit_out -= size_out;
      offset_out += size_out;
    } else {
      size_nor = snprintf(data_nor + offset_nor, limit_nor, "%.*s[%d,%d],",
                          5, g_WdgEntity->modules[i].name,
                          g_WdgEntity->modules[i].time_out,
                          g_WdgEntity->modules[i].last_feed_time);
      limit_nor -= size_nor;
      offset_nor += size_nor;
    }
  }

  data_out[offset_out] = '\0';
  data_nor[offset_nor] = '\0';
#ifndef __FACE__
  LOG_TRACE(LT_SYS, "%04d %s", LOG_ID_WATCHDOG_REBOOT, data_out);
  LOG_TRACE(LT_SYS, "%04d %s", LOG_ID_WATCHDOG_REBOOT, data_nor);
#endif
  DLOG_ERROR(MOD_EB, "%s", data_out);
  DLOG_INFO(MOD_EB, "%s", data_nor);
  printf("\n[wdg]error: %s\n", data_out);
  printf("\n[wdg]info: %s\n", data_nor);
  Wdg_DumpSystemInfo();
  (void)Wdg_PreRebootNotify();
  WatchDog_RebootSystem();
}

void* Wdg_ThreadProc(void* usr_data) {
  (void)Wdg_ThreadConfig();
  if (!Wdg_LoadDefModule()) {
    DLOG_WARNING(MOD_EB, "load default config failed");
  }

  while (!(g_WdgEntity->stop_flag)) {
    vzsleep(WDG_THREAD_PERIODIC);
    (void)Wdg_FeedSysWdg();
    if (g_WdgEntity->reload_cfgfile) {
      g_WdgEntity->reload_cfgfile = false;
      (void)Wdg_LoadDefModule();
    }

    //开关判断
    if (!g_WdgEntity->enable) {
      continue;
    }
    for (long i=0; i<g_WdgEntity->module_num; i++) {
      if(++g_WdgEntity->modules[i].last_feed_time
          > g_WdgEntity->modules[i].time_out) {
        // 模块超时
        Wdg_TimeoutHdlr();
        vzsleep(5*1000);
        break;
      }
    }
  }

  DLOG_INFO(MOD_EB, "watch dog thread exit!!!");
  VZ_FREE(g_WdgEntity);
  g_WdgEntity = NULL;
  return NULL;
}

bool Wdg_ThreadCreate(void) {
#if defined(WIN32)
  HANDLE thread;
  DWORD  thread_id;
  thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Wdg_ThreadProc,
                        NULL, 0, &(thread_id));
  if (!thread) {
    DLOG_ERROR(MOD_EB, "create log server thread failed");
    return false;
  }

  g_WdgEntity->thread_ = thread;
  g_WdgEntity->thread_id_ = thread_id;
#elif defined(POSIX)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int ret = pthread_create(&g_WdgEntity->thread_, &attr, Wdg_ThreadProc, NULL);
  if (0 != ret) {
    DLOG_ERROR(MOD_EB, "create log server thread failed, err:%d", ret);
    return false;
  }
#endif
  return true;
}

void WatchDog_Dump(void) {
  if (!g_WdgEntity) {
    DLOG_ERROR(MOD_EB, "watch dog module uninited");
    return;
  }

  DLOG_INFO(MOD_EB, "total %d modules:", (int)(g_WdgEntity->module_num));
  for (int i=0; i<g_WdgEntity->module_num; i++) {
    DLOG_INFO(MOD_EB, "%32s, %d", g_WdgEntity->modules[i].name,
              g_WdgEntity->modules[i].time_out);
  }
}

int WatchDog_Register(const char *name, unsigned int sec_timeout) {
  do {
    if (!g_WdgEntity) {
      DLOG_ERROR(MOD_EB, "watch dog module uninited");
      break;
    }

    if (!name || (0 == strlen(name))) {
      DLOG_ERROR(MOD_EB, "invalid module name");
      break;
    }

    if (sec_timeout > WDG_MAX_TIMEOUT) {
      DLOG_WARNING(MOD_EB, "module %s invalid time %d, resized to %d",
                   name, sec_timeout, WDG_MAX_TIMEOUT);
      sec_timeout = WDG_MAX_TIMEOUT;
    } else if (sec_timeout < WDG_MIN_TIMEOUT) {
      DLOG_WARNING(MOD_EB, "module %s invalid time %d, resized to %d",
                   name, sec_timeout, WDG_MIN_TIMEOUT);
      sec_timeout = WDG_MIN_TIMEOUT;
    }

    // 查找是否已注册模块
    int pos = -1;
    for (int i=0; i<g_WdgEntity->module_num; i++) {
      if (0 == strcmp(name, g_WdgEntity->modules[i].name)) {
        pos = i;
        break;
      }
    }

    if (-1 == pos) {
      uint32 curr_num = VZ_FAA(&(g_WdgEntity->module_num), 1);
      if ((curr_num + 1) > WDG_MODULE_MAX_NUM) {
        (void)VZ_FAS(&(g_WdgEntity->module_num), 1);
        DLOG_ERROR(MOD_EB, "modules reach max num:%d", WDG_MODULE_MAX_NUM);
        break;
      }
      pos = curr_num;
    }

    snprintf(g_WdgEntity->modules[pos].name, WDG_MODULE_NAME_MAX_LEN, name);
    g_WdgEntity->modules[pos].time_out = sec_timeout;
    g_WdgEntity->modules[pos].registered = true;
    g_WdgEntity->modules[pos].last_feed_time = 0;
    return (pos + WDG_MODULE_KEY_BASE);
  } while (0);

  DLOG_ERROR(MOD_EB, "module register failed");
  return -1;
}

void WatchDog_RebootSystem(void) {
  Log_FlushCache();
  // sleep 3秒，等待Log线程记录日志
  vzsleep(3*1000);
#if defined WIN32
  exit(127);
#elif defined UBUNTU64
  exit(127);
#elif defined LITEOS
  int ret = VZ_DeviceSDK_Sys_Reboot();
  if (0 > ret) {
    DLOG_ERROR(MOD_EB, "reboot system failed");
  }
#else
  system("reboot");
#endif
  vzsleep(1*1000);
}

int WatchDog_FeedDog(unsigned int key) {
  do {
    if (!g_WdgEntity) {
      DLOG_ERROR(MOD_EB, "watch dog module uninited");
      break;
    }
    if (!g_WdgEntity->enable) {
      return 0;
    }

    uint32 pos = key - WDG_MODULE_KEY_BASE;
    if (pos >= WDG_MODULE_MAX_NUM) {
      DLOG_ERROR(MOD_EB, "invalid key %d", key);
      break;
    }

    if (!(g_WdgEntity->modules[pos].registered)) {
      DLOG_ERROR(MOD_EB, "invalid key %d, unregistered module", key);
      break;
    }

    g_WdgEntity->modules[pos].last_feed_time = 0;
    DLOG_DEBUG(MOD_EB, "module \"%s\" feed dog successed",
               g_WdgEntity->modules[pos].name);
    return 0;
  } while (0);

  DLOG_ERROR(MOD_EB, "feed dog failed");
  return -1;
}

int WatchDog_Enable(int enable) {
  if (!g_WdgEntity) {
    DLOG_ERROR(MOD_EB, "Enable(%d) watch dog failed, uninited");
    return -1;
  }
  bool on = enable ? true : false;
  if (on == g_WdgEntity->enable) {
    return 0;
  }
  if (on) {
    for (long i = 0; i < g_WdgEntity->module_num; i++) {
      g_WdgEntity->modules[i].last_feed_time = 0;
    }
  }
  g_WdgEntity->enable = on;
  DLOG_INFO(MOD_EB, "Enable(%d) watch dog successed", g_WdgEntity->enable);
  return 0;
}

int WatchDog_SetPreRebootCb(preRebootCb usr_cb, unsigned int timeout) {
  if (!g_WdgEntity) {
    DLOG_ERROR(MOD_EB, "set user cb failed, watch dog uninited");
    return -1;
  }
  g_WdgEntity->reboot_cb = usr_cb;
  g_WdgEntity->reboot_cb_time = timeout;
  if (WDG_MAX_PREREBOOT_CB_TIMEOUT < timeout) {
    g_WdgEntity->reboot_cb_time = WDG_MAX_PREREBOOT_CB_TIMEOUT;
  }
  return 0;
}

void WatchDog_Deinit(void) {
  if (g_WdgEntity) {
    g_WdgEntity->stop_flag = true;
  }
}

int WatchDog_Init(void) {
  if (g_WdgEntity) {
    // 重新加载配置文件
    g_WdgEntity->reload_cfgfile = true;
    return 0;
  }

  do {
    g_WdgEntity = (WatchDog_Entity*)VZ_MALLOC(sizeof(WatchDog_Entity));
    if (NULL == g_WdgEntity) {
      DLOG_ERROR(MOD_EB, "malloc memory failed,size:%d", sizeof(WatchDog_Entity));
      break;
    }
    memset(g_WdgEntity, 0x00, sizeof(WatchDog_Entity));
    if (!Wdg_ThreadCreate()) {
      break;
    }

    g_WdgEntity->sys_dog_init = Wdg_InitSysWdg();
    g_WdgEntity->reboot_cb = NULL;
    g_WdgEntity->reboot_cb_time = 0;
    g_WdgEntity->enable = true;
    DLOG_INFO(MOD_EB, "watch dog module init successed");
    return 0;
  } while(0);

  DLOG_ERROR(MOD_EB, "watch dog module init failed");
  return -1;
}

