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
#include "eventservice/base/basictypes.h"
#include "appinterface.h"

namespace app {

#define MSG_ID_FEED_WATCHDOG    0x8FFFFFFF

AppWatchdog::AppWatchdog(const std::string name, uint32 timeout,
                         uint32 feed_time)
  : app_name_(name),
    timeout_(timeout),
    feed_time_(feed_time),
    watchdog_key_(-1) {
  stop_feeddog_ = 0;

  DLOG_INFO(MOD_EB, "Module %s init watchdog(tiome_out:%ds, feed_time:%ds)",
            app_name_.c_str(), timeout, feed_time);
}

AppWatchdog::~AppWatchdog() {
}

void AppWatchdog::Start(vzes::EventService::Ptr event_service) {
  if (NULL == event_service) {
    DLOG_ERROR(MOD_EB, "Statr module %s watchdog failed", app_name_.c_str());
    return;
  }

  event_service_ = event_service;
  watchdog_key_ = WatchDog_Register(app_name_.c_str(), timeout_);
  if (watchdog_key_ < 0) {
    DLOG_ERROR(MOD_EB, "Register %s watchdog failed", app_name_.c_str());
    return;
  }
  event_service_->PostDelayed(feed_time_ * 1000, this, MSG_ID_FEED_WATCHDOG);
}

void AppWatchdog::Stop() {
  stop_feeddog_ = 1;
}

void AppWatchdog::OnMessage(vzes::Message* msg) {
  if (1 == stop_feeddog_) {
    return;
  }

  if (msg && (MSG_ID_FEED_WATCHDOG == msg->message_id)) {
    int ret = WatchDog_FeedDog(watchdog_key_);
    if (0 > ret) {
      DLOG_ERROR(MOD_EB, "Module (%s) feed watchdog failed", app_name_.c_str());
    }
    if (event_service_) {
      event_service_->PostDelayed(feed_time_ * 1000, this, MSG_ID_FEED_WATCHDOG);
    }
  }
}

void AppInterface::InitWatchdog(vzes::EventService::Ptr event_service,
                                uint32 timeout, uint32 feed_time) {
  app_watchdog_.reset(new AppWatchdog(app_name_.c_str(), timeout, feed_time));
  app_watchdog_->Start(event_service);
}

}
