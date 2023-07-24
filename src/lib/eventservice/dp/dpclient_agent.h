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
#ifndef EVENTSERVICE_DP_NET_CLIENT_H_
#define EVENTSERVICE_DP_NET_CLIENT_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "log/log/log_client.h"


namespace vzes {

class DpClientAgent : public boost::noncopyable,
  public boost::enable_shared_from_this<DpClientAgent>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<DpClientAgent> Ptr;
  sigslot::signal2<DpClientAgent::Ptr, int> SignalErrorEvent;

 public:
  DpClientAgent(EventService::Ptr es, AsyncSocket::Ptr async_socket);
  ~DpClientAgent();

 private:
  void close ();
  bool Init(AsyncSocket::Ptr async_socket);
  void OnDpMessage(DpClient::Ptr dp_client, DpMessage::Ptr dp_msg);
  void OnSocketWriteComplete(AsyncPacketSocket::Ptr async_socket);
  void OnSocketReadComplete(AsyncPacketSocket::Ptr async_socket,
                            MemBuffer::Ptr data, uint16 flag);
  void OnSocketErrorEvent(AsyncPacketSocket::Ptr async_socket, int err);

  bool OnListenMessage(DpNetMsgHeader *dpmsg);
  bool OnRemoveMessage(DpNetMsgHeader *dpmsg);
  bool OnBroadCastMessage(DpNetMsgHeader *dpmsg);
  bool OnRequestMessage(DpNetMsgHeader *dpmsg);
  bool OnReplayMessage(DpNetMsgHeader *dpmsg);
  bool SendMessage(uint32 msg_id, uint8 type, std::string method,
                   const char *data, uint32 data_size, uint32 session_id);

 public:
  static uint8             channel_count_;  // 对象计数
 private:
  typedef std::map<uint32, DpMessage::Ptr> DPRequests;
  DPRequests               requests_;       // DP Request消息缓存
  DpClient::Ptr            dp_client_;      // 本地DP Client对象
  AsyncPacketSocket::Ptr   async_socket_;   // Tcp Socket
  EventService::Ptr        event_service_;  // Service线程
  uint32                   message_id_;     // DP Message、Request消息计数,自增
};

}

#endif  // EVENTSERVICE_DP_NET_CLIENT_H_

