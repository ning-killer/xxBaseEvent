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
#include "filecache/server/cachenetserver.h"

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

  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    Log_DbgSetLevel(MOD_EB, LL_DEBUG);
    vzes::SocketAddress addr("0.0.0.0", 5294);
    cache::InitCacheNetServer(addr);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    while (1) {
      vzsleep(10000);
      vzes::SocketAddress addr("0.0.0.0", 5294);
      cache::ResetCacheNetServer(addr);
    }
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