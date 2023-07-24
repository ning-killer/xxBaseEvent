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

#ifndef EVENTSERVICE_DP_DPSERVER_H_
#define EVENTSERVICE_DP_DPSERVER_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpclient_agent.h"

namespace vzes {

// Initialize the DP syetem.
bool InitDpSystem();
// Initializes the DP message network service to support
// inter-process and inter-device communication.
bool InitDpNetServer(const SocketAddress addr);
// Reset the DP message network server when the local IP address changes.
bool ResetDpNetServer(const SocketAddress addr);

class DpNetServer : public boost::noncopyable,
  public boost::enable_shared_from_this<DpNetServer>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<DpNetServer> Ptr;

 public:
  DpNetServer();
  ~DpNetServer();

  // Start the service.
  bool Start(const SocketAddress addr);
  // Reset the local IP address.
  bool Reset(const SocketAddress addr);

 protected:
  void OnListenerAcceptEvent(AsyncListener::Ptr listener, Socket::Ptr s, int err);
  void OnAgentErrorEvent(DpClientAgent::Ptr dp_agent, int err);

 private:
  typedef std::list<DpClientAgent::Ptr> DpAgents;
  DpAgents             dp_agents_;
  EventService::Ptr    event_service_;
  AsyncListener::Ptr   async_listener_;
  SocketAddress        address_;
 public:
  static DpNetServer  *instance_;
};

}

#endif  // EVENTSERVICE_DP_DPSERVER_H_
