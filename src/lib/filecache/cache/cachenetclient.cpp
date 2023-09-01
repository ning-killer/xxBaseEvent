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

#include "filecache/cache/cacheclient.h"
#include "filecache/server/cacheserver.h"
#include "eventservice/net/networktinterfaceimpl.h"
#include "eventservice/net/asyncpacketsocket.h"

namespace cache {

vzes::EventService::Ptr GetNetServiceEventService();
#define WAITSIGNALTIME (1000)

class CacheNetClientImpl : public CacheClient,
  public boost::noncopyable,
  public boost::enable_shared_from_this<
  CacheNetClientImpl>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<CacheNetClientImpl> Ptr;
  CacheNetClientImpl() {
    session_id_ = 0;
    current_wait_session_id_ = 0;
    response_type_ = 0;
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
  }

  ~CacheNetClientImpl() {
    //ap_socket_->Close();
    DLOG_INFO(MOD_EB, "Close CacheNetClient");
    vzes::CritScope cr(&crit_);
    if(ap_socket_) {
      ap_socket_->Close();
      ap_socket_.reset();
    }
    if(async_connect_) {
      async_connect_->Close();
      async_connect_.reset();
    }
  };
  bool Init(const vzes::SocketAddress addr) {
    DLOG_INFO(MOD_EB, "Cache net client init request");
    vzes::EventService::Ptr service_es = GetNetServiceEventService();
    async_connect_ = service_es->CreateAsyncConnect();
    if (NULL == async_connect_.get()) {
      DLOG_ERROR(MOD_EB, "Create async connector failed");
      return false;
    }
    async_connect_->SignalServerConnected.
    connect(this, &CacheNetClientImpl::OnSocketConnectEvent);
    if (!async_connect_->Connect(addr, 1000)) {
      return false;
    }
    int res = signal_event_->WaitSignal(WAITSIGNALTIME);
    if (res != SIGNAL_EVENT_DONE) {
      if (res == SIGNAL_EVENT_FAILURE) {
        DLOG_WARNING(MOD_EB, "Connect remote address %s error",
                     addr.ToString().c_str());
      } else {
        DLOG_WARNING(MOD_EB, "Connect remote address %s timeout",
                     addr.ToString().c_str());

      }
      return false;
    }
    return true;
  }

  void OnSocketConnectEvent(vzes::AsyncConnecter::Ptr async_client,
                            vzes::Socket::Ptr s, int err) {
    if (err || !s) {
      DLOG_ERROR(MOD_EB, "Cache Net Client connect remote failed %d", err);
      return;
    }
    DLOG_INFO(MOD_EB, "Cache Net Client connect remote succeed:%s",
              s->GetRemoteAddress().ToString().c_str());

    vzes::EventService::Ptr service_es = GetNetServiceEventService();
    vzes::AsyncSocket::Ptr async_socket = service_es->CreateAsyncSocket(s);
    vzes::AsyncPacketSocket::Ptr ap_socket(
      new vzes::AsyncPacketSocket(service_es, async_socket));
    if (NULL == ap_socket.get()) {
      DLOG_ERROR(MOD_EB, "Create async connector failed");
      return;
    }
    ap_socket->SignalPacketWrite.connect(
      this, &CacheNetClientImpl::OnSocketWriteComplete);
    ap_socket->SignalPacketEvent.connect(
      this, &CacheNetClientImpl::OnSocketReadComplete);
    ap_socket->SignalPacketError.connect(
      this, &CacheNetClientImpl::OnSocketErrorEvent);
    ap_socket->AsyncRead();
    ap_socket_ = ap_socket;
    DLOG_KEY(MOD_EB, "locl addr(%s), remote addr(%s)",
             ap_socket_->local_addr().ToString().c_str(),
             ap_socket_->remote_addr().ToString().c_str());
    signal_event_->TriggerSignal();
  }

  void OnSocketWriteComplete(vzes::AsyncPacketSocket::Ptr async_socket) {
    DLOG_DEBUG(MOD_EB, "Socket send data done");
  }

  void OnSocketReadComplete(vzes::AsyncPacketSocket::Ptr async_socket,
                            MemBuffer::Ptr data, uint16 flag) {
    DLOG_DEBUG(MOD_EB, "Socket read data done");
    std::string packet = data->ToString();
    if (packet.size() < sizeof(CacheNetMessage)) {
      DLOG_ERROR(MOD_EB, "Invalid Cache message length:expected:%d,find:%d",
                 sizeof(CacheNetMessage), packet.size());
    } else {
      CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
      if (msg->id != current_wait_session_id_) {
        DLOG_WARNING(MOD_EB, "msg_id error expected:%d find %d",
                     current_wait_session_id_, msg->id);
        (void) ap_socket_->AsyncRead();
        return;
      }
      switch (msg->type) {
      case NET_TYPE_WRITE: {
        OnWriteResponseMessage(packet);
        break;
      }
      case NET_TYPE_READ: {
        OnReadResponseMessage(packet);
        break;
      }
      case NET_TYPE_DELETE: {
        OnDeleteResponseMessage(packet);
        break;
      }
      default: {
        DLOG_ERROR(MOD_EB,
                   "Invalid Cache message received, type:%d",
                   msg->type);
        break;
      }
      }
      response_type_ = msg->response_type;
    }
    (void) ap_socket_->AsyncRead();
    signal_event_->TriggerSignal();

  }

  void OnSocketErrorEvent(vzes::AsyncPacketSocket::Ptr async_socket,
                          int err) {
    DLOG_INFO(MOD_EB, "cachenetclient socket disconnected. err : %d", err);

    {
      vzes::CritScope cr(&crit_);
      ap_socket_->Close();
    }
    SignalNetBreakEvent(shared_from_this(), err);
  }

  void OnWriteResponseMessage(const std::string &packet) {
    CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
    if (packet.length() < sizeof(CacheNetMessage) + msg->path_len) {
      DLOG_ERROR(MOD_EB,
                 "Write Response Message Length error.expected:%d,find:%d",
                 sizeof(CacheNetMessage) + msg->path_len,
                 packet.length());
    }
    char *path = (char *) packet.c_str() + sizeof(CacheNetMessage);
    if (msg->response_type) {
      MemBuffer::Ptr tmp_buffer = MemBuffer::CreateMemBuffer();
      tmp_buffer->WriteBytes(path, msg->path_len);
      tmp_buffer_ = tmp_buffer;
    } else {
      tmp_buffer_.reset();
    }
  }

  void OnReadResponseMessage(const std::string &packet) {
    CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
    if (packet.length() < sizeof(CacheNetMessage) + msg->data_size) {
      DLOG_ERROR(MOD_EB,
                 "Read Response Message Length error.expected:%d,find:%d",
                 sizeof(CacheNetMessage) + msg->data_size,
                 packet.length());
    }
    char *data = (char *) packet.c_str() + sizeof(CacheNetMessage);
    if (msg->response_type) {
      MemBuffer::Ptr tmp_buffer = MemBuffer::CreateMemBuffer();
      tmp_buffer->WriteBytes(data, msg->data_size);
      tmp_buffer_ = tmp_buffer;
    } else {
      tmp_buffer_.reset();
    }
  }

  void OnDeleteResponseMessage(const std::string &packet) {
    return;
  }

  bool SendData(vzes::MemBuffer::Ptr buffer) {
    vzes::CritScope cr(&crit_);
    return ap_socket_->AsyncWritePacket(
             buffer->ToString().c_str(), buffer->size(), 0);
  }

  bool Write(const char *file_name, const char *data, int data_size,
             char path[128]) {
    CacheNetMessage net_msg;
    net_msg.type = NET_TYPE_WRITE;
    net_msg.id = ++session_id_;
    net_msg.data_size = data_size;
    net_msg.file_name_len = strlen(file_name);

    MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
    buffer->WriteBytes((char *) &net_msg, sizeof(CacheNetMessage));
    buffer->WriteBytes(data, data_size);
    buffer->WriteBytes(file_name, net_msg.file_name_len);
    signal_event_->ResetTriggerSignal();
    current_wait_session_id_ = session_id_;
    if(!SendData(buffer)) {
      return false;
    }

    int res = signal_event_->WaitSignal(WAITSIGNALTIME);
    if (res != SIGNAL_EVENT_DONE) {
      DLOG_WARNING(MOD_EB,
                   "cachenetclient Write Error.WaitSignal return %d",
                   res);
      return false;
    }
    if (response_type_) {
      tmp_buffer_->ReadBytes(path, tmp_buffer_->size());
    }
    return response_type_;
  }

  virtual bool Write(const char *file_name, vzes::MemBuffer::Ptr data,
                     char path[128]) {
    CacheNetMessage net_msg;
    net_msg.type = NET_TYPE_WRITE;
    net_msg.id = ++session_id_;
    net_msg.data_size = data->size();
    net_msg.file_name_len = strlen(file_name);

    MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
    buffer->WriteBytes((char *) &net_msg, sizeof(CacheNetMessage));
    buffer->AppendBuffer(data);
    buffer->WriteBytes(file_name, net_msg.file_name_len);
    signal_event_->ResetTriggerSignal();
    current_wait_session_id_ = session_id_;
    if(!SendData(buffer)) {
      return false;
    }

    int res = signal_event_->WaitSignal(WAITSIGNALTIME);
    if (res != SIGNAL_EVENT_DONE) {
      DLOG_WARNING(MOD_EB,
                   "cachenetclient Write Error.WaitSignal return %d",
                   res);
      return false;
    }
    memset(path, 0, 128);
    if (response_type_) {
      tmp_buffer_->ReadBytes(path, tmp_buffer_->size());
    }
    return response_type_;
  }

  virtual MemBuffer::Ptr Read(const char *path) {
    CacheNetMessage net_msg;
    net_msg.type = NET_TYPE_READ;
    net_msg.path_len = strlen(path);
    net_msg.id = ++session_id_;

    MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
    buffer->WriteBytes((char *) &net_msg, sizeof(CacheNetMessage));
    buffer->WriteBytes(path, net_msg.path_len);
    signal_event_->ResetTriggerSignal();
    current_wait_session_id_ = session_id_;
    if(!SendData(buffer)) {
      return MemBuffer::Ptr();
    }

    int res = signal_event_->WaitSignal(WAITSIGNALTIME);
    if (res != SIGNAL_EVENT_DONE) {
      DLOG_WARNING(MOD_EB,
                   "cachenetclient Read Error.WaitSignal return %d",
                   res);
      return MemBuffer::Ptr();
    }
    vzes::BlocksPtr &block_list = tmp_buffer_->blocks();
    vzes::Block::Ptr block = block_list.back();
    if (((int)(unsigned char)block->buffer[block->buffer_size - 1] != 0xD9)) {
      DLOG_WARNING(MOD_EB, "read file %s from cache_net,size:%d,last byte:%X",
                   path, tmp_buffer_->size(),
                   (int)(unsigned char)block->buffer[block->buffer_size - 1]);
    }
    return tmp_buffer_;
  }

  virtual bool Delete(const char *path) {
    CacheNetMessage net_msg;
    net_msg.type = NET_TYPE_DELETE;
    net_msg.path_len = strlen(path);
    net_msg.id = ++session_id_;

    MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
    buffer->WriteBytes((char *) &net_msg, sizeof(CacheNetMessage));
    buffer->WriteBytes(path, net_msg.path_len);
    current_wait_session_id_ = session_id_;
    signal_event_->ResetTriggerSignal();
    if(!SendData(buffer)) {
      return false;
    }

    int res = signal_event_->WaitSignal(WAITSIGNALTIME);
    if (res != SIGNAL_EVENT_DONE) {
      DLOG_WARNING(MOD_EB,
                   "cachenetclient Delete Error.WaitSignal return %d",
                   res);
      return false;
    }
    return response_type_;
  }

  virtual void ReleaseCache() {
    //not implement
    return;
  }

  virtual void SetPathMode(bool use_abs_path) {
    //not implement
    return;
  }

 public:
  static vzes::EventService::Ptr service_es_;
 private:
  vzes::CriticalSection crit_;
  vzes::SignalEvent::Ptr signal_event_;
  vzes::AsyncPacketSocket::Ptr ap_socket_;
  vzes::AsyncConnecter::Ptr async_connect_;

  uint32 session_id_;
  vzes::MemBuffer::Ptr tmp_buffer_;
  bool response_type_;
  uint32 current_wait_session_id_;
};

vzes::EventService::Ptr
CacheNetClientImpl::service_es_ = vzes::EventService::Ptr();

vzes::EventService::Ptr GetNetServiceEventService() {
  if (CacheNetClientImpl::service_es_.get() == NULL) {
    CacheNetClientImpl::service_es_ =
      vzes::EventService::CreateEventService(NULL,
          "vz_FileCacheNetClientSrv");
    if (CacheNetClientImpl::service_es_.get() == NULL) {
      DLOG_ERROR(MOD_EB, "Create vz_FileCacheNetClientSrv failed.");
    }
  }
  return CacheNetClientImpl::service_es_;
}

cache::CacheClient::Ptr CacheClient::CreateCacheClient(
  vzes::SocketAddress addr) {
  CacheNetClientImpl::Ptr client(new CacheNetClientImpl());
  if (!client->Init(addr)) {
    DLOG_ERROR(MOD_EB, "CreateCacheClient failed");
    return cache::CacheClient::Ptr();
  }
  return client;
}
} //cache
