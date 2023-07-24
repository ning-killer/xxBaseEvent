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
#ifndef FILECACHE_CACHE_CLIENT_AGENT_H__CLIENT_H_
#define FILECACHE_CACHE_CLIENT_AGENT_H__CLIENT_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterface.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "cacheclient.h"

namespace cache {

class CacheClientAgent : public boost::noncopyable,
  public boost::enable_shared_from_this<CacheClientAgent>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<CacheClientAgent> Ptr;
  sigslot::signal2<CacheClientAgent::Ptr, int> SignalErrorEvent;

 public:
  CacheClientAgent(vzes::EventService::Ptr es,
                   vzes::AsyncSocket::Ptr async_socket);
  ~CacheClientAgent();

 private:
  bool Init(vzes::AsyncSocket::Ptr async_socket);
  void OnSocketWriteComplete(vzes::AsyncPacketSocket::Ptr async_socket);
  void OnSocketReadComplete(vzes::AsyncPacketSocket::Ptr async_socket,
                            vzes::MemBuffer::Ptr data, uint16 flag);
  void OnSocketErrorEvent(vzes::AsyncPacketSocket::Ptr async_socket, int err);

  void onWriteMessage(const vzstd::string &packet);
  void onReadMessage(const vzstd::string &packet);
  void onDeleteMessage(const vzstd::string &packet);

 public:
  static uint8             channel_id_;
 private:
  CacheClient::Ptr            cache_client_;
  vzes::AsyncPacketSocket::Ptr   async_socket_;
  vzes::EventService::Ptr        event_service_;
  uint32                   session_id_;
};

}
#endif  // FILECACHE_CACHE_CLIENT_AGENT_H__CLIENT_H_
