/* * vzsdk
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
#include <iostream>
#include "app/app/app.h"
#include "app/app/appstarup.h"
#include "log/monitor/watchdog.h"

// 定义回调函数
void CallBack() {
  DLOG_INFO(MOD_EB, "this is callback code");
  // 在设定的时间上，有5秒的额外时间
  uint32 time_test = 8*1000;
  if (time_test > 2 * 1000) {
    DLOG_INFO(MOD_EB, "timeout test");
    vzsleep(time_test);
    DLOG_INFO(MOD_EB, "timeout failed! thread still runing");
  }
  while (1) {
    static int i = 0;
    i++;
  }
  vzsleep(time_test);
  DLOG_INFO(MOD_EB, "callbcall exit!!");
  return;
}

class WatchDogApp : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<WatchDogApp>,
  public sigslot::has_slots<> {
 public:
  WatchDogApp() :AppInterface("WatchDogTestApp") {
  }
  virtual ~WatchDogApp() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    key_1_ = WatchDog_Register("eventServerApp", 67);
    key_2_ = WatchDog_Register("tcp_server", 77);
    key_3_ = WatchDog_Register("http_sender", 30);
    key_4_ = WatchDog_Register("system_server", 4);

    key_5_ = WatchDog_Register("filecachedserver", 67);
    key_6_ = WatchDog_Register("dispatcher_server", 77);
    key_7_ = WatchDog_Register("dploggingservices", 30);
    key_8_ = WatchDog_Register("play_server", 4);

    key_9_  = WatchDog_Register("devicegroup_server", 67);
    key_10_ = WatchDog_Register("businessserver", 77);
    key_11_ = WatchDog_Register("av_server.out", 30);
    key_12_ = WatchDog_Register("boa", 4);

    key_13_ = WatchDog_Register("alg_proc", 30);
    key_14_ = WatchDog_Register("onvifserver", 4);

    // 设置回调函数和回调时间
    (void)WatchDog_SetPreRebootCb(CallBack, 2 * 1000);
    WatchDog_Enable(false);
    WatchDog_Dump();
    event_service_->PostDelayed(4*1000, this, 0);
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

  virtual void OnMessage(vzes::Message *msg) {
    WatchDog_FeedDog(key_1_);
    WatchDog_FeedDog(key_2_);
    WatchDog_FeedDog(key_3_);
    WatchDog_FeedDog(key_4_);
    WatchDog_FeedDog(key_5_);
    WatchDog_FeedDog(key_6_);
    WatchDog_FeedDog(key_7_);
    WatchDog_FeedDog(key_8_);
    WatchDog_FeedDog(key_9_);
    WatchDog_FeedDog(key_10_);
    WatchDog_FeedDog(key_11_);
    WatchDog_FeedDog(key_12_);
    WatchDog_FeedDog(key_13_);
    WatchDog_FeedDog(key_14_);

    event_service_->PostDelayed(4*1000, this, 0);
    WatchDog_Enable(true);
  }

 private:
  vzes::EventService::Ptr event_service_;
  int key_1_;
  int key_2_;
  int key_3_;
  int key_4_;
  int key_5_;
  int key_6_;
  int key_7_;
  int key_8_;
  int key_9_;
  int key_10_;
  int key_11_;
  int key_12_;
  int key_13_;
  int key_14_;
};

int main(void) {
  app::App::Ptr app = app::App::CreateApp();
  Log_SetPrintEnd(LT_DEBUG, LE_LOCAL | LE_FILE);
  Log_SetPrintEnd(LT_DEV, LE_LOCAL| LE_FILE);
  app::AppInterface::Ptr watchdog(new WatchDogApp());
  app->RegisterApp(watchdog);
  app->AppRun();
  LOG_TRACE(LT_DEV, "DEV TRACE test!");
  LOG_TRACE(LT_POINT, "POINT TRACE test!");
  LOG_TRACE(LT_SYS, "SYS TRACE test!");
  LOG_TRACE(LT_UI, "UI TRACE test!");
  app->ExitApp();
  return EXIT_SUCCESS;
}
