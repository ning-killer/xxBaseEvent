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

#include "app/app/appstarup.h"

#include <iostream>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/base/timeutils.h"
#include "filecache/kvdb/kvdbclient.h"
#include "filecache/cache/cacheclient.h"
#include "log/log/log_client.h"
#include <stdio.h>


#ifdef POSIX
#include <time.h>
#include <sys/prctl.h>
#include <sys/time.h>
#endif

class SerchThread : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<SerchThread>,
  public sigslot::has_slots<> {
 public:
  SerchThread() :AppInterface("SerchThread") {
  }
  virtual ~SerchThread() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    search_count_  = 0;
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    event_service_->PostDelayed(5000, this);
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  virtual void OnMessage(vzes::Message* msg) {
    search_count_++;
    if (1 == search_count_) {
      int result_count = 0;
      char buffer[1000][LOG_RELEASE_MAX_LEN];
      LOG_TIME_S start, end, last_time;
      uint8 type_mask = LT_POINT | LT_SYS | LT_UI | LT_DEV;
      
      vzes::TimeVal tv;
      vzes::TimeOfDay(&tv, NULL);
      end.sec = tv.sec;
      end.usec = tv.usec;
      start.sec = tv.sec - 10;
      start.usec = tv.usec;
      last_time.sec = 0;
      last_time.usec = 0;

      result_count = Log_Search(type_mask, start, end, 1000, buffer, last_time);
      printf(">>>> Log_Search count-1: %d\n", result_count);

      type_mask = LT_POINT | LT_SYS;
      result_count = Log_Search(type_mask, last_time, end, 1000, buffer, last_time);
      printf("<<<< Log_Search count-2: %d\n", result_count);

      type_mask = LT_POINT | LT_SYS | LT_DEV;
      result_count = Log_Search(type_mask, last_time, end, 1000, buffer, last_time);
      printf(">>>> Log_Search count-3: %d\n", result_count);

      type_mask = LT_POINT | LT_SYS | LT_DEV;
      result_count = Log_Search(type_mask, last_time, end, 1000, buffer, last_time);
      printf("<<<< Log_Search count-4: %d\n", result_count);
      search_count_ = 0;
    }
    event_service_->PostDelayed(2000, this);
  }

 private:
  vzes::EventService::Ptr event_service_;
  int search_count_;
};


class EchoClient : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoClient>,
  public sigslot::has_slots<> {
 public:
  EchoClient() :AppInterface("EchoClient") {
  }
  virtual ~EchoClient() {
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
    log_key_ = Log_DbgModRegist((char*)"echo_client", LL_INFO);
    DLOG_INFO(log_key_, "echo_client registered, id=%d", log_key_);
    //Log_SetPrintEnd(LT_POINT, LE_LOCAL);
    //Log_SetPrintEnd(LT_DEBUG, LE_LOCAL);
    event_service_->PostDelayed(5000, this);

    int log_key_1 = Log_DbgModRegist((char*)"echo_client_1", LL_INFO);
    int log_key_2 = Log_DbgModRegist((char*)"echo_client_2", LL_INFO);
    int log_key_3 = Log_DbgModRegist((char*)"echo_client_3", LL_INFO);
    int log_key_4 = Log_DbgModRegist((char*)"echo_client_4", LL_INFO);
    int log_key_5 = Log_DbgModRegist((char*)"echo_client_5", LL_INFO);
    int log_key_6 = Log_DbgModRegist((char*)"echo_client_6", LL_INFO);
    int log_key_7 = Log_DbgModRegist((char*)"echo_client_7", LL_INFO);
    int log_key_8 = Log_DbgModRegist((char*)"echo_client_8", LL_INFO);
    int log_key_9 = Log_DbgModRegist((char*)"echo_client_9", LL_INFO);
    int log_key_10 = Log_DbgModRegist((char*)"echo_client_10", LL_INFO);
    int log_key_11 = Log_DbgModRegist((char*)"echo_client_11", LL_INFO);
    int log_key_12 = Log_DbgModRegist((char*)"echo_client_12", LL_INFO);
    int log_key_13 = Log_DbgModRegist((char*)"echo_client_13", LL_INFO);
    int log_key_14 = Log_DbgModRegist((char*)"echo_client_14", LL_INFO);
    int log_key_15 = Log_DbgModRegist((char*)"echo_client_15", LL_INFO);
    int log_key_16 = Log_DbgModRegist((char*)"echo_client_16", LL_INFO);
    int log_key_17 = Log_DbgModRegist((char*)"echo_client_17", LL_INFO);
    int log_key_18 = Log_DbgModRegist((char*)"echo_client_18", LL_INFO);

    (void)Log_DbgSetLevel(log_key_4, LL_WARNING);
    (void)Log_DbgSetLevel(log_key_10, LL_NONE);
    (void)Log_DbgSetLevel(log_key_14, LL_NONE);

    (void)Log_DbgSetLevel(log_key_15, LL_NONE);
    (void)Log_DbgSetLevel(log_key_18, LL_NONE);

    DLOG_INFO(log_key_15, "xxxxxx\n");
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  virtual void OnMessage(vzes::Message* msg) {
    for (int i=0; i<1; i++) {
#if 0
      struct timeval tv_start, tv_stop;
      gettimeofday(&tv_start, NULL);
      DLOG_INFO(log_key_, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, %d", i);
      gettimeofday(&tv_stop, NULL);
      printf(">>>>50B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      DLOG_INFO(log_key_, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb, %d", i);
      gettimeofday(&tv_stop, NULL);
      printf(">>>>100B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      DLOG_INFO(log_key_, "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccc, %d", i);
      gettimeofday(&tv_stop, NULL);
      printf(">>>>256B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      DLOG_INFO(log_key_, "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc, %d", i);
      gettimeofday(&tv_stop, NULL);
      printf(">>>>1k, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      DLOG_INFO(log_key_, "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc, %d", i);
      gettimeofday(&tv_stop, NULL);
      printf(">>>>2k, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));


      gettimeofday(&tv_start, NULL);
      LOG(L_INFO) << "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, " << i;
      gettimeofday(&tv_stop, NULL);
      printf("****50B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      LOG(L_INFO) << "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                  << "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb, "
                  << i;
      gettimeofday(&tv_stop, NULL);
      printf("****100B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      LOG(L_INFO) << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccc "
                  << i;
      gettimeofday(&tv_stop, NULL);
      printf("****256B, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      LOG(L_INFO) << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << i;
      gettimeofday(&tv_stop, NULL);
      printf("****1k, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));

      gettimeofday(&tv_start, NULL);
      LOG(L_INFO) << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
                  << i;
      gettimeofday(&tv_stop, NULL);
      printf("****2k, usec:%d\n",
             (tv_stop.tv_sec - tv_start.tv_sec)*1000*1000
             + (tv_stop.tv_usec - tv_start.tv_usec));
#endif

      DLOG_ERROR(log_key_, "1111, error error error, %d", i);
      DLOG_INFO(log_key_, "1111, info info info, eeeeeeeeccccccccccccccccccccc"
                "cccccccccccccccccccccyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyxxxxxx"
                "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "xxxxxxxxxxxxxxxxxxxxxxxxxxxrrrrrr, %d", i);
      LOG_TRACE(LT_DEV, "1111, trace trace trace, %d", i);
      LOG_POINT(6, 66, "1111, eeeeeeeecccccccccccccccccccccccccccccccccccccccccc"
                "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                "rrrrrr, %d", i);
      LOG_POINT(10, 256, NULL);

#if 0
      int result_count = 0;
      char buffer[10][LOG_RELEASE_MAX_LEN];
      LOG_TIME_S start, end, last_time;
      uint8 type_mask = LT_POINT | LT_SYS | LT_UI | LT_DEV;

      vzes::TimeVal tv;
      vzes::TimeOfDay(&tv, NULL);
      end.sec = tv.sec;
      end.usec = tv.usec;
      start.sec = tv.sec - 15;
      start.usec = tv.usec;
      last_time.sec = 0;
      last_time.usec = 0;

      result_count = Log_Search(type_mask, start, end,  10, buffer, last_time);
      printf(">>>> Log_Search count:%d\n", result_count);
      for (int i=0; i<result_count; i++) {
        LOG_NODE_S *log_node = (LOG_NODE_S *)buffer[i];
        vzes::TimeLocal time;
        vzes::TimeMkLocal(&time, log_node->sec);
        (void)printf(">>>> %4d-%02d-%02d %02d:%02d:%02d,%-6d,%s\n",
                     time.year, time.month, time.day, time.hour, time.min,
                     time.sec, tv.tv_usec, (char*)log_node + sizeof(LOG_NODE_S));
      }
#endif
    }

    event_service_->PostDelayed(500, this);
  }

 private:
  vzes::EventService::Ptr event_service_;
  int log_key_;
};

class EchoServer : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoServer>,
  public sigslot::has_slots<> {
 public:
  EchoServer() :AppInterface("EchoServer") {
  }
  virtual ~EchoServer() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    vzes::TimeVal tv;
    vzes::TimeOfDay(&tv, NULL);
    printf(">>>> sec:%02d, usec:%-6d\n", tv.sec, tv.usec);

    vzes::TimeLocal time;
    vzes::TimeMkLocal(&time, tv.sec);
    (void)printf(">>>> %4d-%02d-%02d %02d:%02d:%02d,%-6d\n",
                 time.year, time.month, time.day, time.hour,
                 time.min, time.sec, tv.usec);

    vzes::TimeLocal time_2;
    time_2.usec = 0;
    time_2.sec = 50;
    time_2.min = 10;
    time_2.hour = 12;
    time_2.day = 26;
    time_2.month = 10;
    time_2.year = 2018;
    time_2.wday = 5;

    long sec = vzes::TimeMkUTC(time);
    printf(">>>> sec:%02d\n", sec);

    
    vzes::TimeMkLocal(&time, sec);
    (void)printf(">>>> %4d-%02d-%02d %02d:%02d:%02d,%-6d\n",
                 time.year, time.month, time.day, time.hour,
                 time.min, time.sec, tv.usec);
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    log_key_ = Log_DbgModRegist((char*)"echo_client", LL_INFO);
    DLOG_INFO(log_key_, "echo_client registered, id=%d", log_key_);
    event_service_->PostDelayed(2300, this);
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  virtual void OnMessage(vzes::Message* msg) {
    for (int i=0; i<6; i++) {
      DLOG_DEBUG(log_key_, "22222, debug debug debug, %d", i);
      DLOG_WARNING(log_key_, "22222, WARNING WARNING WARNING, %d", i);
      DLOG_INFO(log_key_, "22222, info info info, %d", i);
      LOG_TRACE(LT_DEV, "22222, trace trace trace, %d", i);
      LOG_POINT(6, 66, "22222, point point point, %d", i);
      LOG_POINT(10, 256, NULL);
    }

    event_service_->PostDelayed(300, this);
  }

 private:
  vzes::EventService::Ptr event_service_;
  int log_key_;
};

int main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr echo_client(new EchoClient());
  app::AppInterface::Ptr echo_server(new EchoServer());
  app::AppInterface::Ptr search_thread(new SerchThread());
  app->RegisterApp(echo_client);
  app->RegisterApp(echo_server);
  app->RegisterApp(search_thread);
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}
