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
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpclient_agent.h"
#include "eventservice/net/networktinterfaceimpl.h"
#include "astl/mem_dump.h"


namespace vzes {

uint8 DpClientAgent::channel_count_ = 0;

DpClientAgent::DpClientAgent(EventService::Ptr es,
                             AsyncSocket::Ptr async_socket)
  : message_id_(0),
    event_service_(es) {
  ASSERT_RETURN_VOID(!es);
  ASSERT_RETURN_VOID(!async_socket);
  Init(async_socket);
  channel_count_++;
  DLOG_INFO(MOD_EB, "DpClientAgent construct, channel count:%d", channel_count_);
}

DpClientAgent::~DpClientAgent() {
  channel_count_--;
  DLOG_INFO(MOD_EB, "DpClientAgent destruct, channel count:%d", channel_count_);
  close();
}

bool DpClientAgent::Init(AsyncSocket::Ptr async_socket) {
  AsyncPacketSocket::Ptr ap_socket(
    new AsyncPacketSocket(event_service_, async_socket));
  if (NULL == ap_socket.get()) {
    DLOG_ERROR(MOD_EB, "Create async packet socket failed");
    return false;
  }
  ap_socket->SignalPacketWrite.connect(
    this, &DpClientAgent::OnSocketWriteComplete);
  ap_socket->SignalPacketEvent.connect(
    this, &DpClientAgent::OnSocketReadComplete);
  ap_socket->SignalPacketError.connect(
    this, &DpClientAgent::OnSocketErrorEvent);
  ap_socket->AsyncRead();

  async_socket_ = ap_socket;
  dp_client_ = DpClient::CreateDpClient(event_service_);
  dp_client_->SignalDpMessage.connect(
    this, &DpClientAgent::OnDpMessage);
  DLOG_INFO(MOD_EB, "Dp client agent init successed");
  return true;
}

void DpClientAgent::close () {
  requests_.clear();
  if (async_socket_.get()) {
    async_socket_->Close();
    async_socket_.reset();
  }
  if (dp_client_.get()) {
    DpClient::DestoryDpClient(dp_client_);
    dp_client_.reset();
  }
  DLOG_INFO(MOD_EB, "DpClientAgent closed");
}

void DpClientAgent::OnSocketWriteComplete(
  AsyncPacketSocket::Ptr async_socket) {
  //DLOG_INFO(MOD_EB, "Socket send data done");
}

void DpClientAgent::OnSocketReadComplete(
  AsyncPacketSocket::Ptr async_socket, MemBuffer::Ptr data, uint16 flag) {
  vzstd::string packet = data->ToString();
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
  case TYPE_ADD_MESSAGE: {
    (void)OnListenMessage(dpmsg);
    break;
  }
  case TYPE_REMOVE_MESSAGE: {
    (void)OnRemoveMessage(dpmsg);
    break;
  }
  default: {
    DLOG_INFO(MOD_EB, "Invalid Dp message received, type:%d", dpmsg->type);
    break;
  }
  }

  (void)async_socket_->AsyncRead();
}

void DpClientAgent::OnSocketErrorEvent(
  AsyncPacketSocket::Ptr async_socket, int err) {
  close();
  SignalErrorEvent(shared_from_this(), err);
}

bool DpClientAgent::OnListenMessage(DpNetMsgHeader *dpmsg) {
  std::string method;
  method.append(dpmsg->body, dpmsg->method_size);
  dp_client_->ListenMessage(method);
  return true;
}

bool DpClientAgent::OnRemoveMessage(DpNetMsgHeader *dpmsg) {
  std::string method;
  method.append(dpmsg->body, dpmsg->method_size);
  dp_client_->RemoveMessage(method);
  return true;
}

bool DpClientAgent::OnBroadCastMessage(DpNetMsgHeader *dpmsg) {
  std::string method;
  method.append(dpmsg->body, dpmsg->method_size);
  char *data = dpmsg->body + dpmsg->method_size;
  uint32 data_size = NetworkToHost32(dpmsg->data_size);
  return dp_client_->SendDpMessage(method, 0, data, data_size);
}

bool DpClientAgent::OnRequestMessage(DpNetMsgHeader *dpmsg) {
  DpBuffer res_buffer;
  std::string method;
  method.append(dpmsg->body, dpmsg->method_size);
  char *data = dpmsg->body + dpmsg->method_size;
  uint32 data_size = NetworkToHost32(dpmsg->data_size);
  bool res = dp_client_->SendDpRequest(method, dpmsg->session_id, data,
                                       data_size, &res_buffer, dpmsg->timeout);
  if (!res) {
    DLOG_ERROR(MOD_EB, "Send DP Request failed:%s", method.c_str());
  }
  res = SendMessage(dpmsg->message_id, TYPE_REPLY, method, res_buffer.c_str(),
                    res_buffer.size(), dpmsg->session_id);
  return res;
}

bool DpClientAgent::OnReplayMessage(DpNetMsgHeader *dpmsg) {
  DPRequests::iterator iter = requests_.find(dpmsg->message_id);
  if (iter != requests_.end()) {
    DpMessage::Ptr dp_msg = iter->second;
    requests_.erase(dpmsg->message_id);
    //DLOG_WARNING(MOD_EB, "############: %d", requests_.size());
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
    std::string method;
    method.append(dpmsg->body, dpmsg->method_size);
    DLOG_ERROR(MOD_EB, "Invalid DP Replay msg, id %d, method:%s",
               dpmsg->message_id, method.c_str());
    return false;
  }
}

void DpClientAgent::OnDpMessage(DpClient::Ptr dp_client,
                                DpMessage::Ptr dp_msg) {
  uint32 message_id = ++message_id_;
  bool res = SendMessage(message_id, dp_msg->type, dp_msg->method,
                         (char*)dp_msg->req_buffer.c_str(),
                         dp_msg->req_buffer.size(), dp_msg->session_id);
  if (!res) {
    DLOG_ERROR(MOD_EB, "Transmit DP message to Net client failed");
    return;
  }

  if (TYPE_REQUEST == dp_msg->type) {
    requests_[message_id] = dp_msg;
  }
}

bool DpClientAgent::SendMessage(uint32 msg_id, uint8 type, std::string method,
                                const char *data, uint32 data_size, uint32 session_id) {
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
  dpmsg->method_size = method_len;
  dpmsg->session_id = session_id;
  dpmsg->timeout = 0;
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

}

