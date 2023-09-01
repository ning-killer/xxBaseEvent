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
