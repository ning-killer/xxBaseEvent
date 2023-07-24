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


#include <stdio.h>
#include "app/app/app.h"
#include "filecache/cache/cacheclient.h"
#include "filecache/server/cachenetserver.h"

char *filename[2] = {"filename1.txt", "filename2.txt"};
char *filedata[2] = {"ABCDEFGHTIGSDFSDFHSDFDSFSDFSDGSDGFSDGHSDFasdfasdfasdfaf",
                     "adsfasdfasdfasdfasdfadsfadsfadsfadsfadsfasdasdfadsfasad"};
char *truehost = "127.0.0.1";
//char *truehost = "192.168.108.84";
char *errorhost = "192.160.2.1";
int trueport = 5294;
int errorport = 5293;

class FilecacheNetClientApp : public app::AppInterface,
                              public boost::noncopyable,
                              public boost::enable_shared_from_this<
                                  FilecacheNetClientApp>,
                              public sigslot::has_slots<> {
 public:
  FilecacheNetClientApp() : AppInterface("FileCacheNetClientApp") {

  }
  virtual ~FilecacheNetClientApp() {
  }

  bool connect_test1() {
    DLOG_KEY(MOD_EB, "connect_test1 start");
    bool ans = true;
    vzes::SocketAddress addr(truehost, trueport);
    cache::CacheClient::Ptr cc;
    cc = cache::CacheClient::CreateCacheClient(addr);
    if (!cc) {
      ans = false;
      DLOG_ERROR(MOD_EB, "create cachenetclient error.addr(%s)",
                 addr.ToString().c_str());
    }
    addr = vzes::SocketAddress(truehost, errorport);
    cc = cache::CacheClient::CreateCacheClient(addr);
    if (cc) {
      ans = false;
    }
    addr = vzes::SocketAddress(errorhost, trueport);
    cc = cache::CacheClient::CreateCacheClient(addr);
    if (cc) {
      ans = false;
    }
    addr = vzes::SocketAddress(errorhost, errorport);
    cc = cache::CacheClient::CreateCacheClient(addr);
    if (cc) {
      ans = false;
    }
    DLOG_KEY(MOD_EB, "connect_test1 end %s", ans ? "succeed" : "failed");
    return ans;
  }

  bool test_write1() {
    DLOG_KEY(MOD_EB, "test_write1 start");
    bool ans;
    char path[128];
    vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
    mb->WriteBytes(filedata[0], strlen(filedata[0]));
    vzes::SocketAddress addr(truehost, trueport);
    cache::CacheClient::Ptr cache_client_;
    cache_client_ = cache::CacheClient::CreateCacheClient(addr);
    if (!cache_client_) {
      DLOG_ERROR(MOD_EB, "create cachenetclient error");
      return false;
    }
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[1]);
    cache_client_->Write(filename[0], mb, path);
    vzes::MemBuffer::Ptr data_buffer = cache_client_->Read(path);
    if (!data_buffer.get()) {
      DLOG_ERROR(MOD_EB, "Read error");
      ans = false;
    } else if (strcmp(filedata[0], data_buffer->ToString().c_str())) {
      DLOG_ERROR(MOD_EB, "CacheNetClientTest1 failed.");
      DLOG_ERROR(MOD_EB, "\"%s\"", data_buffer->ToString().c_str());
      ans = false;
    } else {
      ans = true;
    }
    DLOG_KEY(MOD_EB, "test_write1 end %s", ans ? "succeed" : "failed");
    return ans;
  }

  bool test_write2() {
    DLOG_KEY(MOD_EB, "test_write2 start");
    bool ans;
    char path[128];
    vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
    mb->WriteBytes(filedata[0], strlen(filedata[0]));
    vzes::MemBuffer::Ptr mb2 = vzes::MemBuffer::CreateMemBuffer();
    mb2->WriteBytes(filedata[1], strlen(filedata[1]));
    vzes::SocketAddress addr(truehost, trueport);
    cache::CacheClient::Ptr cache_client_;
    cache_client_ = cache::CacheClient::CreateCacheClient(addr);
    if (!cache_client_) {
      DLOG_ERROR(MOD_EB, "create cachenetclient error");
      return false;
    }
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[1]);
    bool res1 = cache_client_->Write(filename[0], mb, path);
    bool res2 = cache_client_->Write(filename[0], mb2, path);
    vzes::MemBuffer::Ptr data_buffer = cache_client_->Read(path);
    if (!data_buffer.get()) {
      DLOG_ERROR(MOD_EB, "Read error");
      ans = false;
    } else if (strcmp(filedata[1], data_buffer->ToString().c_str())) {
      DLOG_ERROR(MOD_EB, "CacheNetClientTest1 failed.");
      DLOG_ERROR(MOD_EB, "\"%s\"", data_buffer->ToString().c_str());
      ans = false;
    } else {
      ans = true;
    }
    DLOG_KEY(MOD_EB, "test_write2 end %s", ans ? "succeed" : "failed");
    return ans;
  }

  bool test_read1() {
    DLOG_KEY(MOD_EB, "test_read1 start");
    bool ans = true;
    char path[128];
    vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
    mb->WriteBytes(filedata[0], strlen(filedata[0]));
    vzes::SocketAddress addr(truehost, trueport);
    cache::CacheClient::Ptr cache_client_;
    cache_client_ = cache::CacheClient::CreateCacheClient(addr);
    if (!cache_client_) {
      DLOG_ERROR(MOD_EB, "create cachenetclient error");
      return false;
    }
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[1]);
    cache_client_->Write(filename[0], mb, path);
    vzes::MemBuffer::Ptr data_buffer = cache_client_->Read(path);
    if (!data_buffer.get()) {
      DLOG_ERROR(MOD_EB, "Read error");
      ans = false;
    } else if (strcmp(filedata[0], data_buffer->ToString().c_str())) {
      DLOG_ERROR(MOD_EB, "CacheNetClientTest1 failed.");
      DLOG_ERROR(MOD_EB, "\"%s\"", data_buffer->ToString().c_str());
      ans = false;
    } else {
      ans = true;
    }
    data_buffer = cache_client_->Read("error_path");
    if (data_buffer.get()) {
      DLOG_ERROR(MOD_EB, "Read error");
      DLOG_ERROR(MOD_EB, "\"%s\"", data_buffer->ToString().c_str());
      ans = false;
    }
    DLOG_KEY(MOD_EB, "test_read1 end %s", ans ? "succeed" : "failed");
    return ans;
  }

  bool test_delete1() {
    DLOG_KEY(MOD_EB, "test_delete1 start");
    bool ans = true;
    char path[128];
    vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
    mb->WriteBytes(filedata[0], strlen(filedata[0]));
    vzes::SocketAddress addr(truehost, trueport);
    cache::CacheClient::Ptr cache_client_;
    cache_client_ = cache::CacheClient::CreateCacheClient(addr);
    if (!cache_client_) {
      DLOG_ERROR(MOD_EB, "create cachenetclient error");
      return false;
    }
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[1]);
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[0]);
    cache_client_->Delete(filename[1]);
    DLOG_KEY(MOD_EB, "test_delete1 end %s", ans ? "succeed" : "failed");
    return ans;
  }

  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    Log_DbgSetLevel(MOD_EB, LL_DEBUG);
    vzes::SocketAddress addr("0.0.0.0", trueport);
    //cache::InitCacheNetServer(addr);
    vzsleep(1000);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    do {
      bool err = false;
      if(!connect_test1()){
        err = true;
      }
      if(!test_write1()){
        err = true;
      }
      if(!test_write2()){
        err = true;
      }
      if(!test_read1()){
        err = true;
      }
      if(!test_delete1()){
        err = true;
      }
      if(err){
        break;
      }
      vzsleep(5000);
    } while (1);
    DLOG_ERROR(MOD_EB, "Test Error");
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
  vzes::EventService::Ptr event_service_;
};

int main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr cachenetclient(new FilecacheNetClientApp());

  app->RegisterApp(cachenetclient);
  app->AppRun();
  while (1);
  app->ExitApp();
}
