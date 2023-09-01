
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
#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/net/asyncpacketsocket.h"
#include "cacheclientagent.h"
#include "log/log/log_client.h"

namespace cache {
CacheClientAgent::CacheClientAgent(vzes::EventService::Ptr es,
                                   vzes::AsyncSocket::Ptr async_socket) {
  event_service_ = es;
  ASSERT_RETURN_VOID(!es);
  ASSERT_RETURN_VOID(!async_socket);
  session_id_ = 0;
  Init(async_socket);
}

CacheClientAgent::~CacheClientAgent() {
}

bool CacheClientAgent::Init(vzes::AsyncSocket::Ptr async_socket) {
  vzes::AsyncPacketSocket::Ptr ap_socket(
    new vzes::AsyncPacketSocket(event_service_, async_socket));
  if (NULL == ap_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async packet socket failed");
    return false;
  }
  ap_socket->SignalPacketWrite.connect(
    this, &CacheClientAgent::OnSocketWriteComplete);
  ap_socket->SignalPacketEvent.connect(
    this, &CacheClientAgent::OnSocketReadComplete);
  ap_socket->SignalPacketError.connect(
    this, &CacheClientAgent::OnSocketErrorEvent);
  bool res = ap_socket->AsyncRead();

  async_socket_ = ap_socket;
  cache_client_ = CacheClient::CreateCacheClient();
  DLOG_DEBUG(MOD_EB, "locl addr(%s), remote addr(%s)",
             async_socket_->local_addr().ToString().c_str(),
             async_socket_->remote_addr().ToString().c_str());
  if(res) {
    DLOG_INFO(MOD_EB, "Cache client agent init succeed");
  } else {
    DLOG_INFO(MOD_EB, "Cache client agent init failed");
  }
  return res;
}

void CacheClientAgent::OnSocketWriteComplete(
  vzes::AsyncPacketSocket::Ptr async_socket) {
  DLOG_DEBUG(MOD_EB, "Socket send data done");
}

void CacheClientAgent::OnSocketReadComplete(
  vzes::AsyncPacketSocket::Ptr async_socket,
  vzes::MemBuffer::Ptr data,
  uint16 flag) {
  DLOG_DEBUG(MOD_EB, "Socket recv data");

  std::string packet = data->ToString();
  if (packet.size() < sizeof(CacheNetMessage)) {
    DLOG_ERROR(MOD_EB, "Invalid Cache Net message length:%d", packet.size());
    return;
  }

  CacheNetMessage *cache_net_msg = (CacheNetMessage *) packet.c_str();
  switch (cache_net_msg->type) {
  case NET_TYPE_WRITE: {
    onWriteMessage(packet);
    break;
  }
  case NET_TYPE_READ: {
    onReadMessage(packet);
    break;
  }
  case NET_TYPE_DELETE: {
    onDeleteMessage(packet);
    break;
  }
  default: {
    DLOG_ERROR(MOD_EB,
               "Invalid cache net message received, type:%d",
               cache_net_msg->type);
    break;
  }
  }

  (void) async_socket_->AsyncRead();
}

void CacheClientAgent::onWriteMessage(const std::string &packet) {
  DLOG_DEBUG(MOD_EB, "cacheclientagent onWriteMessage");
  CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
  if (packet.length()
      < sizeof(CacheNetMessage) + msg->data_size + msg->file_name_len) {
    DLOG_ERROR(MOD_EB, "Write Request Message Length error");
  }
  char *data = (char *) (packet.c_str()) + sizeof(CacheNetMessage);
  char *file_name = (char *) (packet.c_str()) +
                    sizeof(CacheNetMessage) + msg->data_size;
  char path[128];

  bool res = cache_client_->Write(file_name, data, msg->data_size, path);

  CacheNetMessage res_msg;
  res_msg.type = msg->type;
  res_msg.response_type = res;
  res_msg.id = msg->id;
  res_msg.path_len = strlen(path);

  MemBuffer::Ptr res_buffer = MemBuffer::CreateMemBuffer();
  res_buffer->WriteBytes((char *) &res_msg, sizeof(res_msg));
  res_buffer->WriteBytes(path, res_msg.path_len);
  async_socket_->AsyncWritePacket(res_buffer, 0);
}

void CacheClientAgent::onReadMessage(const std::string &packet) {
  DLOG_DEBUG(MOD_EB, "cacheclientagent onReadMessage");
  CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
  if (packet.length() < sizeof(CacheNetMessage) + msg->path_len) {
    DLOG_ERROR(MOD_EB, "Read Request Message Length error");
  }
  char *path = (char *) (packet.c_str()) + sizeof(CacheNetMessage);

  MemBuffer::Ptr data = cache_client_->Read(path);

  CacheNetMessage res_msg;
  res_msg.type = msg->type;
  if (data.get()) {
    res_msg.data_size = data->size();
    res_msg.response_type = 1;
  } else {
    res_msg.data_size = 0;
    res_msg.response_type = 0;
  }
  res_msg.id = msg->id;

  MemBuffer::Ptr res_buffer = MemBuffer::CreateMemBuffer();
  res_buffer->WriteBytes((char *) &res_msg, sizeof(res_msg));
  if (data.get()) {
    res_buffer->AppendBuffer(data);
  }
  vzes::BlocksPtr &block_list = res_buffer->blocks();
  vzes::Block::Ptr block = block_list.back();
  if (((int)(unsigned char)block->buffer[block->buffer_size - 1] != 0xD9)) {
    DLOG_WARNING(MOD_EB, "cacheagent send buffer,size:%d,last byte:%X",
                 res_buffer->size(),
                 (int)(unsigned char)block->buffer[block->buffer_size - 1]);
  }
  async_socket_->AsyncWritePacket(res_buffer, 0);
}

void CacheClientAgent::onDeleteMessage(const std::string &packet) {
  DLOG_DEBUG(MOD_EB, "cacheclientagent onDeleteMessage");
  CacheNetMessage *msg = (CacheNetMessage *) packet.c_str();
  if (packet.length() < sizeof(CacheNetMessage) + msg->path_len) {
    DLOG_ERROR(MOD_EB, "Delete Request Message Length error");
  }
  char *path = (char *) (packet.c_str()) + sizeof(CacheNetMessage);

  bool res = cache_client_->Delete(path);

  CacheNetMessage res_msg;
  res_msg.type = msg->type;
  res_msg.response_type = res;
  res_msg.id = msg->id;

  MemBuffer::Ptr res_buffer = MemBuffer::CreateMemBuffer();
  res_buffer->WriteBytes((char *) &res_msg, sizeof(res_msg));
  async_socket_->AsyncWritePacket(res_buffer, 0);
}

void CacheClientAgent::OnSocketErrorEvent(
  vzes::AsyncPacketSocket::Ptr async_socket,
  int err) {
  DLOG_INFO(MOD_EB, "Socket error");
  async_socket_->Close();
  async_socket_.reset();
  cache_client_.reset();
  SignalErrorEvent(shared_from_this(), err);
}

}
