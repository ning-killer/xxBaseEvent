/*
* vzsdk
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

#if defined(POSIX)
#include <signal.h>
#endif // defined POSIX
#if defined(WIN32)
#include <Windows.h>
#include <signal.h>
#endif // defined(WIN32)

#include "app/app/app.h"
#include "filecache/server/cacheserver.h"
#include "eventservice/dp/dpserver.h"
#include "log/monitor/watchdog.h"
#include "log/log/log_client.h"

extern void backtrace_init();

namespace app {

#define ON_MSG_PRE_INIT   1
#define ON_MSG_INIT       2
#define ON_MSG_RUN_APP    3
#define ON_MSG_EXIT       4

static bool signal_exit;

struct AppInstance {
  AppInstance() {}
  ~AppInstance() {
    app.reset();
    es.reset();
  }
  AppInterface::Ptr app;
  vzes::EventService::Ptr es;
};

struct AppData : public vzes::MessageData {
  AppData() {}
  ~AppData() {
    app.reset();
    es.reset();
    signal_event.reset();
  }
  typedef boost::shared_ptr<AppData> Ptr;
  AppInterface::Ptr       app;
  vzes::EventService::Ptr es;
  vzes::SignalEvent::Ptr  signal_event;
};

class AppImpl : public App,
  public boost::noncopyable,
  public boost::enable_shared_from_this<AppImpl>,
  public sigslot::has_slots<>,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<AppImpl> Ptr;
 public:
  AppImpl() {
    // Make sure that ThreadManager is created on the main thread before
    // we start a new thread.
    printf(">>>>>>EventBase init start, Build time:%s, %s\n", __DATE__, __TIME__);
    vzes::ThreadManager::Instance();
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
  }
  virtual ~AppImpl() {
  }
 public:
  bool InitPublicSystem(bool multi_proc_log) {
    // 初始化日志库
    vzes::LogMessage::LogTimestamps(true);
    vzes::LogMessage::LogContext(vzes::LS_SENSITIVE);
    vzes::LogMessage::LogThreads(true);
    ASSERT_RETURN_FAILURE(Log_Init(multi_proc_log) == false, false);
    // 初始化主线程
    main_es_ = vzes::EventService::CreateCurrentEventService("vz_main_thread");
    // 初始化Filecache和KVDB，只需要调用一次就可以了
    ASSERT_RETURN_FAILURE(cache::CacheServer::Instance() == NULL, false);
    // 初始化星形结构
    ASSERT_RETURN_FAILURE(vzes::InitDpSystem() == false, false);
    ASSERT_RETURN_FAILURE(WatchDog_Init() != 0, false);
    return true;
  }
 public:
  virtual bool RegisterApp(AppInterface::Ptr app) {
    ASSERT_RETURN_FAILURE(!app, false);
    ASSERT_RETURN_FAILURE(!main_es_, false);
    AppInstance app_instance;
    app_instance.app = app;
    app_instance.es  =
      vzes::EventService::CreateEventService(NULL,
          app->app_name(),
          app->thread_stack_size());
    apps_.push_back(app_instance);
    return true;
  }

  virtual void AppRun(bool internal_blocking) {
    ASSERT_RETURN_VOID(!main_es_);
    // Run pre init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_PRE_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      DLOG_INFO(MOD_EB, "PreInit message Done:%s", iter->app->app_name().c_str());
    }
    // run init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      DLOG_INFO(MOD_EB, "Init message Done: %s", iter->app->app_name().c_str());
    }
    // run app
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_RUN_APP, app_data);
      DLOG_INFO(MOD_EB, "Run message Done: %s", iter->app->app_name().c_str());
    }
    //
    if (internal_blocking) {
      main_es_->Run();
    }
  }

  void ExitSystem(void) {
    // ...
    //WatchDog_RebootSystem();
    Log_FlushCache();
    // sleep 3秒，等待Log线程记录日志
    vzsleep(3*1000);
#if defined WIN32
    exit(127);
#elif defined UBUNTU64
    exit(127);
#elif defined LITEOS
    // LITEOS not support signal
#else
    exit(0);
#endif
  }

  virtual void ExitApp() {
    vzes::SignalEvent::Ptr exit_signal = vzes::SignalEvent::CreateSignalEvent();
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      iter->es = vzes::EventService::CreateEventService();
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es  = iter->es;
      app_data->signal_event = exit_signal;
      iter->es->Post(this, ON_MSG_EXIT, app_data);
      exit_signal->WaitSignal(100);
      DLOG_INFO(MOD_EB, "Exit Message Done: %s", iter->app->app_name().c_str());
    }
    DLOG_INFO(MOD_EB, "App exited:%d!!!", signal_exit);
    if (!signal_exit) {
      // WIN32:exit app; Linux device:reboot system.
      WatchDog_RebootSystem();
    } else {
      ExitSystem();
    }
  }

 public:
  virtual void OnMessage(vzes::Message *msg) {
    AppData::Ptr app_data = boost::dynamic_pointer_cast<AppData>(msg->pdata);
    if (msg->message_id == ON_MSG_PRE_INIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_PRE_INIT");
      OnPreInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_INIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_INIT");
      OnInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_RUN_APP) {
      DLOG_INFO(MOD_EB, "ON_MSG_RUN_APP");
      OnRunApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_EXIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_EXIT");
      OnExitApp(app_data->es, app_data->app, app_data->signal_event);
    } else {
      DLOG_ERROR(MOD_EB, "Unkown message");
    }
  }

  void OnPreInitApp(vzes::EventService::Ptr es,
                    AppInterface::Ptr app,
                    vzes::SignalEvent::Ptr signal_event) {
    if (!app->PreInit(es)) {
      DLOG_ERROR(MOD_EB, "App PreInit failed:%s", app->app_name().c_str());
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnInitApp(vzes::EventService::Ptr es,
                 AppInterface::Ptr app,
                 vzes::SignalEvent::Ptr signal_event) {
    if (!app->InitApp(es)) {
      DLOG_ERROR(MOD_EB, "App Init failed:%s", app->app_name().c_str());
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnRunApp(vzes::EventService::Ptr es,
                AppInterface::Ptr app,
                vzes::SignalEvent::Ptr signal_event) {
    if (!app->RunAPP(es)) {
      DLOG_ERROR(MOD_EB, "App Run failed:%s", app->app_name().c_str());
      ExitApp();
    } else {
      app->InitWatchdog(es);
    }
  }

  void OnExitApp(vzes::EventService::Ptr es,
                 AppInterface::Ptr app,
                 vzes::SignalEvent::Ptr signal_event) {
    app->OnExitApp(es);
    // 处理完毕，告诉App
    signal_event->TriggerSignal();
  }
 private:
  typedef std::list<AppInstance> Apps;
  Apps apps_;
  vzes::CriticalSection   crit_;
  vzes::EventService::Ptr main_es_;
  vzes::SignalEvent::Ptr  signal_event_;
};

// 捕捉APP退出信号用
struct AppExit {
  AppImpl::Ptr appopt;
} appexit;

static void ExitSigHandler(int signumber) {
  DLOG_INFO(MOD_EB, "Signal %d captured, start to exit App!!!", signumber);
  signal_exit = true;
  appexit.appopt->ExitApp();
}

int ExitSigRegist(int signumber) {
#if defined(WIN32)
  if (SIG_ERR == signal(signumber, ExitSigHandler)) {
    DLOG_ERROR(MOD_EB, "the signal's catching faile!!");
    return -1;
  }
#elif defined(LITEOS)
  // ..
#else
  struct sigaction act, oldact;
  act.sa_handler = ExitSigHandler;
  act.sa_flags = 0;
  if (sigaction(signumber, &act, &oldact)) {
    DLOG_ERROR(MOD_EB, "the signal's catching faile!!");
    return -1;
  }
#endif
  printf("signal catcher test \n");
  return 0;
}


bool ExitSigCatch() {
#if defined(WIN32)
  if (-1 == ExitSigRegist(SIGBREAK)) {
    return false;
  }
#elif defined(LITEOS)
  // ..
#else
  backtrace_init();
  if (-1 == ExitSigRegist(SIGINT)) {
    return false;
  }
  //后台用 SIGUSR1:10
  if (-1 == ExitSigRegist(SIGUSR1)) {
    return false;
  }
#endif
  return true;
}

App::Ptr App::CreateApp(bool multi_proc_log) {
  AppImpl::Ptr app(new AppImpl());
  // 开启app退出信号捕捉，POSIX下是捕捉Ctrl+C，WIN32下是捕捉Ctrl+Break
  // 若设置应用后台运行:kill -10 PID
  appexit.appopt = app;
  signal_exit = false;
  if (!ExitSigCatch()) {
    DLOG_INFO(MOD_EB, "App Catch exit signal catch failed!");
  }
  if (app->InitPublicSystem(multi_proc_log)) {
    return app;
  }
  return App::Ptr();
}
}
