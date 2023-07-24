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
#include "filecache/server/cachenetserver.h"
#include "eventservice/event/thread.h"

namespace cache {

CacheNetServer *CacheNetServer::instance_ = NULL;

CacheNetServer *GetCacheNetServerInstance() {
  if (CacheNetServer::instance_ == NULL) {
    CacheNetServer::instance_ = new CacheNetServer();
  }
  return CacheNetServer::instance_;
}

bool InitCacheNetServer(const vzes::SocketAddress addr) {
  CacheNetServer *instance = GetCacheNetServerInstance();
  if (!instance) {
    DLOG_ERROR(MOD_EB, "Init CacheNetServer failed.");
    return false;
  }
  return instance->Start(addr);
}

bool ResetCacheNetServer(const vzes::SocketAddress addr) {
  if (NULL == CacheNetServer::instance_) {
    DLOG_ERROR(MOD_EB, "CacheNetServer not inited, reset failed");
    return false;
  }
  return CacheNetServer::instance_->Reset(addr);
}

CacheNetServer::CacheNetServer() {
  DLOG_INFO(MOD_EB, "CacheNetServer construct");
}

CacheNetServer::~CacheNetServer() {
  DLOG_INFO(MOD_EB, "CacheNetServer destruct");
}

bool CacheNetServer::Reset(const vzes::SocketAddress addr) {
  DLOG_KEY(MOD_EB, "Reset CacheNetServer. addr: %s.", addr.ToString().c_str());
  if(addr == address_) {
    DLOG_WARNING(MOD_EB, "addr is same as current address_, Reset failed");
    return false;
  }
  if (async_listener_) {
    async_listener_->Close();
    async_listener_.reset();
  }
  address_.Clear();
  return Start(addr);
}

bool CacheNetServer::Start(const vzes::SocketAddress addr) {
  bool res = true;
  if (!event_service_.get()) {
    event_service_ =
      vzes::EventService::CreateEventService(NULL, "vz_CacheNetSrv");
    ASSERT_RETURN_FAILURE(!event_service_.get(), false);
    event_service_->SetThreadPriority(vzes::PRIORITY_IDLE);
  }
  if (NULL == async_listener_.get()) {
    async_listener_ = event_service_->CreateAsyncListener();
    ASSERT_RETURN_FAILURE(!async_listener_.get(), false);
    async_listener_->SignalNewConnected.connect(
      this, &CacheNetServer::OnListenerAcceptEvent);
    res = async_listener_->Start(addr, true);
    if (res) {
      address_ = addr;
      DLOG_KEY(MOD_EB, "Start CacheNetServer succeed(%s)",
               addr.ToString().c_str());
    } else {
      async_listener_->Close();
      async_listener_.reset();
      DLOG_ERROR(MOD_EB, "Start CacheServer failed(%s)",
                 addr.ToString().c_str());
    }
  }
  return res;
}

void CacheNetServer::OnListenerAcceptEvent(vzes::AsyncListener::Ptr listener,
    vzes::Socket::Ptr s, int err) {
  DLOG_KEY(MOD_EB, "Cache Net Server accept remote client:%s",
           s->GetRemoteAddress().ToString().c_str());
  vzes::AsyncSocket::Ptr async_socket = event_service_->CreateAsyncSocket(s);
  if (NULL == async_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async socket failed");
    return;
  }
  CacheClientAgent::Ptr cache_agent =
    CacheClientAgent::Ptr(new CacheClientAgent(event_service_, async_socket));
  if (NULL == cache_agent.get()) {
    DLOG_ERROR(MOD_EB, "Create Cache Client agent failed");
    return;
  }
  cache_agent->SignalErrorEvent.connect(this,
                                        &CacheNetServer::OnSocketErrorEvent);
  cache_agents_.push_back(cache_agent);
}

void CacheNetServer::OnSocketErrorEvent(CacheClientAgent::Ptr cache_agent,
                                        int err) {
  DLOG_ERROR(MOD_EB, "Cache Agent error:%d", err);
  CacheAgents::iterator pos = std::find(cache_agents_.begin(),
                                        cache_agents_.end(),
                                        cache_agent);
  if (pos != cache_agents_.end()) {
    cache_agents_.erase(pos);
    DLOG_INFO(MOD_EB, "Delete cache agent from server");
  } else {
    DLOG_ERROR(MOD_EB, "Not find the cache agent");
  }
}

}

