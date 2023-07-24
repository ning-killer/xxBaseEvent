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
#include "eventservice/dp/dpserver.h"
#include "eventservice/net/networktinterfaceimpl.h"


namespace vzes {

DpNetServer *DpNetServer::instance_ = NULL;

DpNetServer *GetDpNetServerInstance() {
  if (NULL == DpNetServer::instance_) {
    DpNetServer::instance_ = new DpNetServer();
  }
  return DpNetServer::instance_;
}

bool InitDpNetServer(const SocketAddress addr) {
  DpNetServer *Instance = GetDpNetServerInstance();
  if (!Instance) {
    DLOG_ERROR(MOD_EB, "Init DpNetServer failed");
    return false;
  }
  return DpNetServer::instance_->Start(addr);
}

bool ResetDpNetServer(const SocketAddress addr) {
  if (NULL == DpNetServer::instance_) {
    DLOG_ERROR(MOD_EB, "DpNetServer uninited, reset failed");
    return false;
  }
  return DpNetServer::instance_->Reset(addr);
}

DpNetServer::DpNetServer() {
  DLOG_INFO(MOD_EB, "DpNetServer construct");
}

DpNetServer::~DpNetServer() {
  DLOG_INFO(MOD_EB, "DpNetServer destruct");
  dp_agents_.clear();
  if (event_service_.get()) {
    event_service_.reset();
  }
  if (async_listener_.get()) {
    async_listener_->Close();
    async_listener_.reset();
  }
}

bool DpNetServer::Reset(const SocketAddress addr) {
  DLOG_KEY(MOD_EB, "Reset DpNetServer request(new:%s)",
           addr.ToString().c_str());
  if (addr == address_) {
    DLOG_ERROR(MOD_EB, "Reset DpNetServer failed, addr same as current");
    return false;
  }
  if (async_listener_.get()) {
    async_listener_->Close();
    async_listener_.reset();
  }
  address_.Clear();
  bool res = Start(addr);
  if (res) {
    DLOG_INFO(MOD_EB, "Reset DpNetServer successed");
    return true;
  }
  DLOG_ERROR(MOD_EB, "Reset DpNetServer failed");
  return false;
}

bool DpNetServer::Start(const SocketAddress addr) {
  bool res = true;
  if (NULL == event_service_.get()) {
    event_service_ = EventService::CreateEventService(NULL, "vz_DpNetSrv");
    ASSERT_RETURN_FAILURE((NULL == event_service_.get()), false);
    event_service_->SetThreadPriority(PRIORITY_IDLE);
  }
  if (NULL == async_listener_.get()) {
    async_listener_ = event_service_->CreateAsyncListener();
    ASSERT_RETURN_FAILURE((NULL == async_listener_.get()), false);
    async_listener_->SignalNewConnected.connect(
      this, &DpNetServer::OnListenerAcceptEvent);
    res = async_listener_->Start(addr, true);
    if (res) {
      address_ = addr;
      DLOG_INFO(MOD_EB, "Start DpNetServer successed(%s)",
                addr.ToString().c_str());
    } else {
      async_listener_->Close();
      async_listener_.reset();
      DLOG_ERROR(MOD_EB, "Start DpNetServer failed(%s)",
                 addr.ToString().c_str());
    }
  }
  return res;
}

void DpNetServer::OnListenerAcceptEvent(AsyncListener::Ptr listener,
                                        Socket::Ptr s, int err) {
  DLOG_KEY(MOD_EB, "Dp Net Server accept remote client:%s",
           s->GetRemoteAddress().ToString().c_str());
  AsyncSocket::Ptr async_socket = event_service_->CreateAsyncSocket(s);
  if (NULL == async_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async socket failed");
    return;
  }
  DpClientAgent::Ptr dp_agent =
    DpClientAgent::Ptr(new DpClientAgent(event_service_, async_socket));
  if (NULL == dp_agent.get()) {
    DLOG_ERROR(MOD_EB, "Create Dp Client agent failed");
    return;
  }
  dp_agent->SignalErrorEvent.connect(this, &DpNetServer::OnAgentErrorEvent);
  dp_agents_.push_back(dp_agent);
}

void DpNetServer::OnAgentErrorEvent(DpClientAgent::Ptr dp_agent, int err) {
  DLOG_ERROR(MOD_EB, "Dp Agent error:%d", err);
  DpAgents::iterator pos = std::find(dp_agents_.begin(),
                                     dp_agents_.end(),
                                     dp_agent);
  if (pos != dp_agents_.end()) {
    dp_agents_.erase(pos);
  } else {
    DLOG_ERROR(MOD_EB, "Not find the Dp agents");
  }
}

}

