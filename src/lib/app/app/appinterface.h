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

#ifndef APP_APP_APPINTERFACE_H_
#define APP_APP_APPINTERFACE_H_

#include "eventservice/base/basictypes.h"
#include "astl/include/string.hpp"
#include "eventservice/net/eventservice.h"
#include "log/monitor/watchdog.h"


namespace app {

class AppWatchdog : public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<AppWatchdog> Ptr;
 public:
  // name: 看门狗模块名称
  // timeout: 超时时间，单位秒
  // feed_time: 定时喂狗时间，单位秒
  AppWatchdog(const std::string name, uint32 timeout = WDG_DEF_TIMEOUT,
              uint32 feed_time = WDG_DEF_FEEDDOG_TIME);
  ~AppWatchdog();

  // 启动定时喂狗
  void Start(vzes::EventService::Ptr event_service);
  void Stop();

 protected:
  virtual void OnMessage(vzes::Message* msg);

 private:
  vzes::EventService::Ptr event_service_;
  std::string           app_name_;
  uint32                  timeout_;
  uint32                  feed_time_;
  int                     watchdog_key_;
  unsigned int            stop_feeddog_;
};

class AppInterface {
 public:
  typedef boost::shared_ptr<AppInterface> Ptr;
 public:
  AppInterface(const std::string app_name, size_t thread_stack_size = 0)
    : app_name_(app_name), thread_stack_size_(thread_stack_size) {
  }
  // 各应用准备星形结构、KVDB、初始化数据库等自身环境，应用之间不要相互依赖初始化
  virtual bool PreInit(vzes::EventService::Ptr event_service) = 0;
  // 应用之间可以相互之间初始化，在这个阶段各个应用自身已经初始化成功
  virtual bool InitApp(vzes::EventService::Ptr event_service) = 0;
  // 正式运行
  virtual bool RunAPP(vzes::EventService::Ptr event_service) = 0;
  // This function will be called by other thread
  virtual void OnExitApp(vzes::EventService::Ptr event_service) = 0;
  const std::string app_name() const {
    return app_name_;
  }
  const size_t thread_stack_size() {
    return thread_stack_size_;
  }
  virtual void  InitWatchdog(vzes::EventService::Ptr event_service,
                             uint32 timeout = WDG_DEF_TIMEOUT,
                             uint32 feed_time = WDG_DEF_FEEDDOG_TIME);
 protected:
  std::string app_name_;
  size_t thread_stack_size_;
  AppWatchdog::Ptr app_watchdog_;
};

}

#endif  // APP_APP_APPINTERFACE_H_
