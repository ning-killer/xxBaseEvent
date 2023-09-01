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
#include "map"
#include "eventservice/dp/dpclient.h"
#include "eventservice/net/networktinterfaceimpl.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "astl/mem_dump.h"

namespace vzes {

#define CLIENT_INIT_WAIT_TIME    (3*1000)
#define SERVER_REQ_WAIT_TIME     (10*1000)

EventService::Ptr GetServiceEventService();

class DpNetClientImpl : public DpClient,
  public boost::noncopyable,
  public sigslot::has_slots<>,
  public MessageHandler,
  public boost::enable_shared_from_this<DpNetClientImpl>  {
 public:
  typedef boost::shared_ptr<DpNetClientImpl> Ptr;

 public:
  DpNetClientImpl(EventService::Ptr es)
    : event_service_(es),
      message_id_(0) {
    channel_count_++;
    client_signal_ = vzes::SignalEvent::CreateSignalEvent();
    server_signal_ = vzes::SignalEvent::CreateSignalEvent();
    DLOG_INFO(MOD_EB, "DpNetClient construct, channel count:%d", channel_count_);
  }

  ~DpNetClientImpl() {
    channel_count_--;
    DLOG_INFO(MOD_EB, "DpNetClient destruct, channel count:%d", channel_count_);
    Close();
  }

  bool Init(const SocketAddress addr) {
    DLOG_INFO(MOD_EB, "DpNetClient init request");
    EventService::Ptr service_es = GetServiceEventService();
    async_connect_ = service_es->CreateAsyncConnect();
    if (NULL == async_connect_.get()) {
      DLOG_ERROR(MOD_EB, "Create async connector failed");
      return false;
    }
    async_connect_->SignalServerConnected.connect(this,
        &DpNetClientImpl::OnSocketConnectEvent);
    bool res = async_connect_->Connect(addr, 10*1000);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Async connector Connect failed");
      return false;
    }

    int ret = client_signal_->WaitSignal(CLIENT_INIT_WAIT_TIME);
    if (ret != SIGNAL_EVENT_DONE) {
      client_signal_->ResetTriggerSignal();
      DLOG_INFO(MOD_EB, "DpNetClient init failed, err:%d", ret);
      return false;
    } else {
      DLOG_INFO(MOD_EB, "DpNetClient init successed");
      return true;
    }
  }

  void Close() {
    DLOG_INFO(MOD_EB, "Close DpNetClient");
    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    vzes::CritScope cr(&crit_);
    requests_.clear();
    if (async_socket_) {
      async_socket_->Close();
      async_socket_.reset();
    }
    if (async_connect_) {
      async_connect_->Close();
      async_connect_.reset();
    }
    if (!SignalErrorEvent.is_empty()) {
      SignalErrorEvent.disconnect_all();
    }
    if (!SignalDpMessage.is_empty()) {
      SignalDpMessage.disconnect_all();
    }
  }

  virtual bool SendDpMessage(const std::string method, uint32 session_id,
                             const char *data, uint32 data_size) {
    if (0 == method.size()) {
      DLOG_ERROR(MOD_EB, "Send Dp msg failed, method length: 0");
      return false;
    }

    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    vzes::CritScope cr(&crit_);
    bool res = SendMessage(++message_id_, TYPE_MESSAGE, method,
                           (char*)data, data_size, session_id);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Send Dp msg \"%s\" failed", method.c_str());
    }
    return res;
  }

  virtual bool SendDpRequest(const std::string method, uint32 session_id,
                             const char *data, uint32 data_size,
                             DpBuffer *res_buffer, uint32 timeout) {
    if (0 == method.size()) {
      DLOG_ERROR(MOD_EB, "Send Dp request failed, method length: 0");
      return false;
    }

    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    vzes::CritScope cr(&crit_);
    DpMessage::Ptr dp_msg(new DpMessage);
    dp_msg->method = method;
    dp_msg->session_id = session_id;
    dp_msg->type = TYPE_REQUEST;
    //dp_msg->req_buffer.append(data, data_size);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->isTimeOut = false;
    dp_msg->signal_event = client_signal_;
    uint32 message_id = ++message_id_;
    requests_[message_id] = dp_msg;
    bool ret = SendMessage(message_id, TYPE_REQUEST, method, data,
                           data_size, session_id, timeout);
    if (!ret) {
      DLOG_ERROR(MOD_EB, "Send Dp request failed");
      return false;
    }

    int res = client_signal_->WaitSignal(timeout);
    if (SIGNAL_EVENT_DONE != res) {
      dp_msg->isTimeOut = true;
      client_signal_->ResetTriggerSignal();
      DLOG_ERROR(MOD_EB, "Wait DP Replay failed, res:%d", res);
      requests_.erase(message_id);
      return false;
    } else {
      if (res_buffer) {
        *res_buffer = *dp_msg->res_buffer;
      }
      requests_.erase(message_id);
      return true;
    }
  }

  virtual bool SendDpReply(DpMessage::Ptr dp_msg) {
    if (dp_msg->type != TYPE_REQUEST) {
      DLOG_ERROR(MOD_EB, "Send DpReply failed, msg type %d is not REQUEST",
                 dp_msg->type);
      return false;
    }
    if (dp_msg->isTimeOut) {
      return false;
    } else {
      dp_msg->type = TYPE_REPLY;
      dp_msg->signal_event->TriggerSignal();
      return true;
    }
  }

  virtual void ListenMessage(const std::string method) {
    if (0 == method.size()) {
      DLOG_ERROR(MOD_EB, "Add listen msg failed, msg length: 0");
      return;
    }
    bool res = SendMessage(++message_id_, TYPE_ADD_MESSAGE, method, NULL, 0, 0);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Add listen msg \"%s\" failed", method.c_str());
    } else {
      DLOG_INFO(MOD_EB, "Add listen msg \"%s\" successed", method.c_str());
    }
  }

  virtual void RemoveMessage(const std::string method) {
    if (0 == method.size()) {
      DLOG_ERROR(MOD_EB, "Remove listen msg failed, msg length: 0");
      return;
    }
    bool res = SendMessage(++message_id_, TYPE_REMOVE_MESSAGE, method, NULL, 0, 0);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Remove listen msg \"%s\" failed", method.c_str());
    } else {
      DLOG_INFO(MOD_EB, "Remove listen msg \"%s\" successed", method.c_str());
    }
  }

 private:
  void OnSocketConnectEvent(vzes::AsyncConnecter::Ptr async_client,
                            vzes::Socket::Ptr s, int err) {
    if (err || !s) {
      DLOG_ERROR(MOD_EB, "Connect remote failed");
      return;
    }
    DLOG_INFO(MOD_EB, "Connect remote %s successed",
              s->GetRemoteAddress().ToString().c_str());

    EventService::Ptr service_es = GetServiceEventService();
    AsyncSocket::Ptr async_socket = service_es->CreateAsyncSocket(s);
    AsyncPacketSocket::Ptr ap_socket(
      new AsyncPacketSocket(service_es, async_socket));
    if (NULL == ap_socket.get()) {
      DLOG_ERROR(MOD_EB, "Create async connector failed");
      return;
    }
    ap_socket->SignalPacketWrite.connect(
      this, &DpNetClientImpl::OnSocketWriteComplete);
    ap_socket->SignalPacketEvent.connect(
      this, &DpNetClientImpl::OnSocketReadComplete);
    ap_socket->SignalPacketError.connect(
      this, &DpNetClientImpl::OnSocketErrorEvent);
    ap_socket->AsyncRead();
    async_socket_ = ap_socket;
    client_signal_->TriggerSignal();
  }

  void OnSocketWriteComplete(AsyncPacketSocket::Ptr async_socket) {
    //DLOG_INFO(MOD_EB, "Socket send data done");
  }

  void OnSocketReadComplete(AsyncPacketSocket::Ptr async_socket,
                            MemBuffer::Ptr data, uint16 flag) {
    std::string packet = data->ToString();
    if (packet.size() <= sizeof(DpNetMsgHeader)) {
      DLOG_ERROR(MOD_EB, "Invalid Dp message length:%d", packet.size());
      return;
    }

    DpNetMsgHeader *dpmsg = (DpNetMsgHeader*)packet.c_str();
    switch (dpmsg->type) {
    case TYPE_MESSAGE: {
      (void)OnBroadCastMessage(dpmsg);
      break;
    }
    case TYPE_REQUEST: {
      (void)OnRequestMessage(dpmsg);
      break;
    }
    case TYPE_REPLY: {
      (void)OnReplayMessage(dpmsg);
      break;
    }
    default: {
      DLOG_INFO(MOD_EB, "Invalid Dp message received, type:%d", dpmsg->type);
      break;
    }
    }

    (void)async_socket_->AsyncRead();
  }

  void OnSocketErrorEvent(AsyncPacketSocket::Ptr async_socket, int err) {
    DLOG_INFO(MOD_EB, "AsyncPacketSocket error:%d", err);
    // 避免Socket发生异常时，Service线程和用户线程并发，
    // 对象被销毁导致异常
    vzes::CritScope cr(&crit_);
    requests_.clear();
    if (async_socket_) {
      async_socket_->Close();
      async_socket_.reset();
    }
    SignalErrorEvent(shared_from_this(), TYPE_ERROR_DISCONNECTED);
  }

  virtual void OnMessage(Message *msg) {
    DpMessage::Ptr dp_msg = boost::dynamic_pointer_cast<DpMessage>(msg->pdata);
    SignalDpMessage(shared_from_this(), dp_msg);
  }

 private:
  bool OnBroadCastMessage(DpNetMsgHeader *dpmsg) {
    DpMessage::Ptr dp_msg(new DpMessage);
    dp_msg->method.append(dpmsg->body, dpmsg->method_size);
    dp_msg->session_id = 0;
    dp_msg->type = TYPE_MESSAGE;
    char *data = dpmsg->body + dpmsg->method_size;
    uint32 data_size = NetworkToHost32(dpmsg->data_size);
    dp_msg->req_buffer.append(data, data_size);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->signal_event = client_signal_;

    event_service_->Post(this, 0, dp_msg);
    return true;
  }

  bool OnRequestMessage(DpNetMsgHeader *dpmsg) {
    DpMessage::Ptr dp_msg(new DpMessage);
    dp_msg->method.append(dpmsg->body, dpmsg->method_size);
    dp_msg->session_id = dpmsg->session_id;
    dp_msg->type = TYPE_REQUEST;
    char *data = dpmsg->body + dpmsg->method_size;
    uint32 data_size = NetworkToHost32(dpmsg->data_size);
    dp_msg->req_buffer.append(data, data_size);
    dp_msg->tmp_res = "";
    dp_msg->res_buffer = &dp_msg->tmp_res;
    dp_msg->isTimeOut = false;
    dp_msg->signal_event = server_signal_;

    event_service_->Post(this, 0, dp_msg);
    int res = dp_msg->signal_event->WaitSignal(SERVER_REQ_WAIT_TIME);
    if (SIGNAL_EVENT_DONE != res) {
      dp_msg->isTimeOut = true;
      server_signal_->ResetTriggerSignal();
    }

    bool ret = SendMessage(dpmsg->message_id, TYPE_REPLY, dp_msg->method,
                           dp_msg->res_buffer->c_str(), dp_msg->res_buffer->size(),
                           dpmsg->session_id);
    if (!ret) {
      DLOG_ERROR(MOD_EB, "Transmit DP Replay to server failed");
      return false;
    }
    return ((SIGNAL_EVENT_DONE == res) ? true : false);
  }

  bool OnReplayMessage(DpNetMsgHeader *dpmsg) {
    DPRequests::iterator iter = requests_.find(dpmsg->message_id);
    if (iter != requests_.end()) {
      DpMessage::Ptr dp_msg = iter->second;
      if (dp_msg->type != TYPE_REQUEST) {
        DLOG_ERROR(MOD_EB, "Invalid DP Replay msg, msg type %d is not REQUEST",
                   dp_msg->type);
        return false;
      }
      if(dp_msg->isTimeOut) {
        return false;
      } else {
        char *data = dpmsg->body + dpmsg->method_size;
        uint32 data_size = NetworkToHost32(dpmsg->data_size);
        dp_msg->res_buffer->append(data, data_size);
        dp_msg->type = TYPE_REPLY;
        dp_msg->signal_event->TriggerSignal();
        return true;
      }
    } else {
      return false;
    }
  }

  bool SendMessage(uint32 msg_id, uint8 type, std::string method,
                   const char *data, uint32 data_size, uint32 session_id,
                   uint32 timeout = 0) {
    if (NULL == async_socket_.get()) {
      DLOG_ERROR(MOD_EB, "Invalid Socket, Send message failed"
                 "(msg_id:%d, type:%d, method:%s)",
                 msg_id, type, method.c_str());
      return false;
    }
    uint32 method_len = method.size();
    uint32 buff_len = sizeof(DpNetMsgHeader) + method_len + data_size;
    char *msg_buff = (char*)VZ_MALLOC(buff_len);
    if (NULL == msg_buff) {
      DLOG_ERROR(MOD_EB, "Malloc memory failed, size:%d", buff_len);
      return false;
    }

    memset(msg_buff, 0x00, buff_len);
    DpNetMsgHeader *dpmsg = (DpNetMsgHeader*)msg_buff;
    dpmsg->type = type;
    dpmsg->message_id = msg_id;
    dpmsg->timeout = timeout;
    dpmsg->method_size = method_len;
    dpmsg->session_id = session_id;
    dpmsg->data_size = HostToNetwork32(data_size);
    memcpy(dpmsg->body, method.c_str(), method_len);
    if (data_size > 0) {
      memcpy((dpmsg->body + method_len), data, data_size);
    }
    bool ret = async_socket_->AsyncWritePacket((const char*)msg_buff, buff_len, 0);
    if (!ret) {
      DLOG_ERROR(MOD_EB, "Send message failed(msg_id:%d, type:%d, method:%s)",
                 msg_id, type, method.c_str());
    }
    VZ_FREE(msg_buff);
    return ret;
  }

 public:
  static EventService::Ptr  service_es_;     // NetClient Service线程
  static uint8              channel_count_;  // 对象计数
 private:
  typedef std::map<uint32, DpMessage::Ptr> DPRequests;
  DPRequests                requests_;       // DP Request消息缓存
  AsyncConnecter::Ptr       async_connect_;  // Tcp Connector
  AsyncPacketSocket::Ptr    async_socket_;   // Tcp Socket
  EventService::Ptr         event_service_;  // NetClient工作线程
  SignalEvent::Ptr          client_signal_;  // 用户线程同步阻塞信号
  SignalEvent::Ptr          server_signal_;  // 服务线程同步阻塞信号
  uint32                    message_id_;     // DP Message、Request消息计数,自增
  CriticalSection           crit_;
};

uint8 DpNetClientImpl::channel_count_ = 0;
EventService::Ptr DpNetClientImpl::service_es_ = EventService::Ptr();

EventService::Ptr GetServiceEventService() {
  if (DpNetClientImpl::service_es_ == NULL) {
    DpNetClientImpl::service_es_ =
      EventService::CreateEventService(NULL, "vz_DpNetClient");
  }
  return DpNetClientImpl::service_es_;
}

DpClient::Ptr DpClient::CreateDpNetClient(EventService::Ptr es,
    const SocketAddress addr) {
  if (NULL == es) {
    DLOG_ERROR(MOD_EB, "Create DpNetClient failed, EventService::Ptr==NULL");
    return DpClient::Ptr();
  }
  DpNetClientImpl::Ptr client(new DpNetClientImpl(es));
  bool res = client->Init(addr);
  if (res) {
    return client;
  } else {
    return DpClient::Ptr();
  }
}

void DpClient::DestoryDpNetClient(DpClient::Ptr dp_client) {
  if (NULL == dp_client) {
    DLOG_ERROR(MOD_EB, "Destory DpNetClient failed, DpClient::Ptr==NULL");
    return;
  }
  DpNetClientImpl::Ptr client =
    boost::dynamic_pointer_cast<DpNetClientImpl>(dp_client);
}

}

