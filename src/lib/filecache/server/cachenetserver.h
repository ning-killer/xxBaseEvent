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

#ifndef FILECACHE_SERVER_CACHENETSERVER_H_
#define FILECACHE_SERVER_CACHENETSERVER_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "filecache/cache/cacheclientagent.h"
#include "eventservice/net/networktinterface.h"

namespace cache {

// 初始化CacheNetServer,只需要初始化一次
// addr: CacheNetServer 监听的地址
// return 初始化结果
bool InitCacheNetServer(const vzes::SocketAddress addr);

// 重置CacheNetServer,绑定新的端口。无法重置为和当前地址一样的地址
// addr: 希望 CacheNetServer 监听的新的地址
// return 重置的结果
bool ResetCacheNetServer(const vzes::SocketAddress addr);

class CacheNetServer :
    public boost::noncopyable,
    public boost::enable_shared_from_this<CacheNetServer>,
    public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<CacheNetServer> Ptr;

 public:
  CacheNetServer();
  ~CacheNetServer();

  bool Start(const vzes::SocketAddress addr);
  bool Reset(const vzes::SocketAddress addr);

 protected:
  void OnListenerAcceptEvent(vzes::AsyncListener::Ptr listener,
                             vzes::Socket::Ptr s,
                             int err);
  void OnSocketErrorEvent(CacheClientAgent::Ptr cache_agent, int err);

 private:
  typedef std::list<CacheClientAgent::Ptr> CacheAgents;
  CacheAgents cache_agents_;
  vzes::EventService::Ptr event_service_;
  vzes::SocketAddress address_;
  vzes::AsyncListener::Ptr async_listener_;
 public:
  static CacheNetServer *instance_;
};

}

#endif  // FILECACHE_SERVER_CACHENETSERVER_H_
