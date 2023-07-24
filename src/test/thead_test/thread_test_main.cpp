/*
 * vzevent
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

#include <iostream>
#include <stdio.h>
#include "log/log/log_client.h"
#include "eventservice/net/eventservice.h"

struct TestMessage : public vzes::MessageData {
  typedef boost::shared_ptr<TestMessage> Ptr;
  uint32 index;
};

class TimeEvent : public vzes::MessageHandler {
 public:
  TimeEvent(vzes::EventService::Ptr main_es)
    : main_es_(main_es) {
  }
  virtual ~TimeEvent() {
  }
  void Start() {
    second_es_ = vzes::EventService::CreateEventService(NULL, "second_es");
    TestMessage::Ptr tmsg(new TestMessage);
    tmsg->index = 0;
    Affinity_test();
    main_es_->PostDelayed(1000, this, 0, tmsg);
  }

  void Affinity_test() {
    uint32 affinity;
    bool res;
    res = main_es_->GetThreadAffinity(affinity);
    printf("main thread last affinity_mask:%d res:%d\n", affinity, res);
    res = main_es_->SetThreadAffinity(1);
    printf("main thread set affinity res:%d\n", res);
    res = main_es_->GetThreadAffinity(affinity);
    printf("main thread cur affinity_mask:%d res:%d\n", affinity, res);

    res = second_es_->GetThreadAffinity(affinity);
    printf("second thread last affinity_mask:%d res:%d\n", affinity, res);
    res = second_es_->SetThreadAffinity(2);
    printf("main thread set affinity res:%d\n", res);
    res = second_es_->GetThreadAffinity(affinity);
    printf("second thread cur affinity_mask:%d res:%d\n", affinity, res);

    res = main_es_->GetThreadAffinity(affinity);
    printf("main thread last affinity_mask:%d res:%d\n", affinity, res);
    res = main_es_->SetThreadAffinity(3);
    printf("main thread set affinity res:%d\n", res);
    res = main_es_->GetThreadAffinity(affinity);
    printf("main thread cur affinity_mask:%d res:%d\n", affinity, res);

    res = second_es_->GetThreadAffinity(affinity);
    printf("second thread last affinity_mask:%d res:%d\n", affinity, res);
    res = second_es_->SetThreadAffinity(3);
    printf("main thread set affinity res:%d\n", res);
    res = second_es_->GetThreadAffinity(affinity);
    printf("second thread cur affinity_mask:%d res:%d\n", affinity, res);
  }
  virtual void OnMessage(vzes::Message *msg) {
    TestMessage::Ptr tmsg = boost::dynamic_pointer_cast<TestMessage>(msg->pdata);
    tmsg->index ++;
    vzes::Thread *current_thread = vzes::Thread::Current();
    if (main_es_->IsThisThread(current_thread)) {
      DLOG_INFO(MOD_EB, "msg:%d,Main thread message, switch to second thread",
                tmsg->index);
      LOG_TRACE(LT_DEV, "DEV TRACE test!");
      LOG_TRACE(LT_POINT, "POINT TRACE test!");
      second_es_->PostDelayed(1000, this, 0, tmsg);
    } else if (second_es_->IsThisThread(current_thread)) {
      DLOG_INFO(MOD_EB, "msg:%d,Second thread message, switch to main thread",
                tmsg->index);
      LOG_TRACE(LT_SYS, "SYS TRACE test!");
      LOG_TRACE(LT_UI, "UI TRACE test!");
      main_es_->Post(this, 0, tmsg);
    } else {
      DLOG_ERROR(MOD_EB, "Error! there is unknow thread run this message");
    }
  }
 private:
  vzes::EventService::Ptr   main_es_;
  vzes::EventService::Ptr   second_es_;

};

int main(void) {

  // Initialize the logging system
  (void)Log_Init(false);
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  Log_SetPrintEnd(LT_DEBUG, LE_LOCAL | LE_FILE);
  Log_SetPrintEnd(LT_DEV, LE_LOCAL | LE_FILE);
  Log_SetPrintEnd(LT_POINT, LE_LOCAL | LE_FILE);
  Log_SetPrintEnd(LT_SYS, LE_LOCAL | LE_FILE);
  Log_SetPrintEnd(LT_UI, LE_LOCAL | LE_FILE);

  vzes::EventService::Ptr main_es
    = vzes::EventService::CreateCurrentEventService("MainThread");

  TimeEvent te(main_es);
  te.Start();
  main_es->Run();
  while (1);
  return EXIT_SUCCESS;
}