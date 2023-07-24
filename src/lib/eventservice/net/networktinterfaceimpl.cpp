/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/
#include <stdio.h>

#include "eventservice/net/networktinterfaceimpl.h"
#include "eventservice/base/base64.h"


namespace vzes {

#define MSG_DATA_SEND_COMPLETE  (101) // Tcp数据包发送完成消息
#define MAX_SEND_BLOCK_COUNT    (8)   // 每次最多可发送的membuffer block数量

uint32 AsyncSocketImpl::blocks_count_ = 0;

AsyncSocketImpl::AsyncSocketImpl(EventService::Ptr es, Socket::Ptr s)
  : event_service_(es),
    socket_(s),
    socket_writeable_(true),
    encode_type_(PKT_ENCODE_NONE) {
  write_buffers_ = MemBuffer::CreateMemBuffer();
  send_buffers_ = MemBuffer::CreateMemBuffer();
}

AsyncSocketImpl::~AsyncSocketImpl() {
  DLOG_INFO(MOD_EB, "Destory AsyncSocketImpl");
  Close();
}

bool AsyncSocketImpl::Init() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_event_, false);
  socket_event_ = event_service_->CreateDispEvent(socket_, DE_READ | DE_CLOSE);
  socket_event_->SignalEvent.connect(this, &AsyncSocketImpl::OnSocketEvent);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return true;
}
// Inherit with AsyncSocket
// Async write the data, if the operator complete, SignalWriteCompleteEvent
// will be called
bool AsyncSocketImpl::AsyncWrite(MemBuffer::Ptr buffer) {
  ASSERT_RETURN_FAILURE(!IsConnected(), false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  {
    vzes::CritScope cr(&crit_);
    write_buffers_->AppendBuffer(buffer);
    blocks_count_ += buffer->blocks().size();
  }
#ifndef WIN32
  WaitToWriteData();
#else
  TryToWriteData(false, true);
#endif
  return true;
}

bool AsyncSocketImpl::AsyncRead() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  socket_event_->AddEvent(DE_READ);
  return event_service_->Add(socket_event_);
}

// Returns the address to which the socket is bound.  If the socket is not
// bound, then the any-address is returned.
SocketAddress AsyncSocketImpl::GetLocalAddress() const {
  return socket_->GetLocalAddress();
}

// Returns the address to which the socket is connected.  If the socket is
// not connected, then the any-address is returned.
SocketAddress AsyncSocketImpl::GetRemoteAddress() const {
  return socket_->GetRemoteAddress();
}

void AsyncSocketImpl::Close() {
  if (socket_) {
    socket_->Close();
    socket_.reset();
  }
  if (socket_event_) {
    event_service_->Remove(socket_event_);
    socket_event_->Close();
    socket_event_.reset();
  }
  RemoveAllSignal();
  if (write_buffers_ && write_buffers_->size()) {
    blocks_count_ -= write_buffers_->blocks().size();
    write_buffers_->Clear();
  }
  if (send_buffers_ && send_buffers_->size()) {
    blocks_count_ -= send_buffers_->blocks().size();
    send_buffers_->Clear();
  }
  socket_writeable_ = false;
}

void AsyncSocketImpl::SetEncodeType(PACKET_ENCODE_TYPE encode_type) {
  encode_type_ = encode_type;
}

int AsyncSocketImpl::GetError() const {
  ASSERT_RETURN_FAILURE(!socket_, 0);
  return socket_->GetError();
}

void AsyncSocketImpl::SetError(int error) {
  ASSERT_RETURN_VOID(!socket_);
  return socket_->SetError(error);
}

bool AsyncSocketImpl::IsConnected() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  return socket_->GetState() != Socket::CS_CLOSED;
}

int AsyncSocketImpl::GetOption(Option opt, int* value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->GetOption(opt, value);
}

int AsyncSocketImpl::SetOption(Option opt, int value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->SetOption(opt, value);
}

void AsyncSocketImpl::OnMessage(vzes::Message *msg) {
  if (msg->message_id == MSG_DATA_SEND_COMPLETE) {
    SocketWriteComplete();
  }
}

///
void AsyncSocketImpl::OnSocketEvent(EventDispatcher::Ptr accept_event,
                                    Socket::Ptr socket,
                                    uint32 event_type,
                                    int err) {
  AsyncSocketImpl::Ptr live_this = shared_from_this();
  //DLOG_INFO(MOD_EB, "Event Type:%d", event_type);
  if(err) {
    DLOG_ERROR(MOD_EB, "Socket error received, err:%d, event:%d",
               err, event_type);
    SocketErrorEvent(err);
    return;
  }
  // socket_event_->RemoveEvent(DE_CLOSE);
  if (socket_event_ && event_type & DE_WRITE) {
    socket_event_->RemoveEvent(DE_WRITE);
    //DLOG_INFO(MOD_EB, "Socket Write Event:%d", event_type);
    SocketWriteEvent();
  }
  if (socket_event_ && event_type & DE_READ) {
    socket_event_->RemoveEvent(DE_READ);
    //DLOG_INFO(MOD_EB, "Socket Read Event:%d", event_type);
    SocketReadEvent();
  }
  if (socket_event_ && event_type & DE_CLOSE) {
    DLOG_ERROR(MOD_EB, "Socket close event received");
    socket_event_->RemoveEvent(DE_CLOSE);
    SocketErrorEvent(err);
  }
  if (socket_event_ && socket_event_->get_enable_events()) {
    event_service_->Add(socket_event_);
  }
}

void AsyncSocketImpl::SocketReadEvent() {

  MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
  int res               = socket_->Recv(buffer);
  int error_code        = socket_->GetError();

  if (res > 0) {
    SocketReadComplete(buffer);
  } else if (res == 0) {
    //DLOG_ERROR(MOD_EB, "%s", evutil_socket_error_to_string(error_code));
    // Socket已经关闭了
    DLOG_ERROR(MOD_EB, "Connection reset by peer");
    //BOOST_ASSERT(error_code == 0);
    // SocketReadComplete(MemBuffer::Ptr(), error_code);
    SocketErrorEvent(error_code);
  } else {
    DLOG_ERROR(MOD_EB, "received data error, error:%d", error_code);
    // 接收数据出问题了
    if (IsBlockingError(error_code)) {
      // 数据阻塞，之后继续接收数据
      return;
    } else {
      // Socket出了问题
      // SocketReadComplete(MemBuffer::Ptr(), error_code);
      DLOG_ERROR(MOD_EB, "socket errored");
      SocketErrorEvent(error_code);
    }
  }
}

void AsyncSocketImpl::SocketWriteEvent() {
  vzes::CritScope cr(&crit_);
  socket_writeable_ = true;
  TryToWriteData(true, false);
}

void AsyncSocketImpl::SocketErrorEvent(int err) {
  AsyncSocket::Ptr live_this = shared_from_this();
  SignalSocketErrorEvent(live_this, err);
  Close();
}

int32 AsyncSocketImpl::TryToWriteData(bool is_emit_close_event,
                                      bool is_sync_call) {
  if (!socket_writeable_) {
    //DLOG_INFO(MOD_EB, "can't write data, waitting to write");
    return -1;
  }

  // 数据已经全部发送完成
  BlocksPtr &write_block_list = write_buffers_->blocks();
  BlocksPtr &send_block_list = send_buffers_->blocks();
  if ((write_buffers_->size() == 0) && (send_buffers_->size() == 0)) {
    SocketWriteComplete();
    return 0;
  }

  while (1) {
    // 发送缓存中的数据发送完，从数据缓存list中获取Blcok发送
    if (send_buffers_->size() == 0) {
      if (write_buffers_->size() == 0) {
        // 数据已经全部发送完成
        if (!is_sync_call) {
          SocketWriteComplete();
        } else {
#ifdef WIN32
          // For windows: An FD_WRITE network event is recorded when a
          // socket is first connected with connect/WSAConnect or accepted
          // with accept/WSAAccept, and then after a send fails with
          // WSAEWOULDBLOCK and buffer space becomes available.
          // So here we simulate FD_WRITE event by post message.
          event_service_->Post(this, MSG_DATA_SEND_COMPLETE);
#else
          WaitToWriteData();
#endif
        }
        break;
      }

      int count = 0;
      for (BlocksPtr::iterator iter = write_block_list.begin();
           iter != write_block_list.end();) {
        Block::Ptr block = *iter;
        if ((block->encode_flag_) && (encode_type_ != PKT_ENCODE_NONE)) {
          // 当前默认为Base64编码
          Base64::EncodeFromArray(block->buffer, block->buffer_size, send_buffers_);
          if (block->buffer_size > (DEFAULT_BLOCK_SIZE / 4 * 3)) {
            blocks_count_ += 1;
          }
        } else {
          send_buffers_->AppendBlock(block);
        }
        write_buffers_->ReduceSize(block->buffer_size);
        write_block_list.erase(iter++);
        if ((++count) >= MAX_SEND_BLOCK_COUNT) {
          break;
        }
      }
    }

    int total_len = send_buffers_->size();
    int total_blocks = send_buffers_->blocks().size();
    int res = socket_->Send(send_buffers_);
    if (res == total_len) {
      // 数据发送完成，继续发送
      blocks_count_ -= total_blocks;
      continue;
    } else if (res > 0 && res < total_len) {
      // 数据没有写完，等待下一次再写入数据
      blocks_count_ -= (total_blocks - send_buffers_->blocks().size());
      WaitToWriteData();
      break;
    } else if (res == 0) {
      // 没有发送任何数据，只有等待下一次再发送
      // 理论上不应该进入这种状态的，除非对方有问题
      WaitToWriteData();
      break;
    } else {
      // 证明数据阻塞了，发送不动
      int error_code = socket_->GetError();
      if (IsBlockingError(error_code)) {
        WaitToWriteData();
      } else {
        // 如果在这次操作过程中不需要发送超时消息，则下一次再发送
        if (is_emit_close_event) {
          // SocketWriteComplete(error_code);
          SocketErrorEvent(error_code);
        } else {
          // Not Emit write error message
          DLOG_WARNING(MOD_EB, "Not Emit error message");
          WaitToWriteData();
        }
      }
    }
    break;
  }
  return 0;
}

void AsyncSocketImpl::WaitToWriteData() {
  socket_writeable_ = false;
  socket_event_->AddEvent(DE_WRITE);
  event_service_->Add(socket_event_);
}

////////////////////////////////////////////////////////////////////////////////

void AsyncSocketImpl::SocketReadComplete(MemBuffer::Ptr buffer) {
  // 确保生命周期内不会因为外部删除而删除了整个对象
  // 如果是Socket Read事件导致的Socket错误，并不会出问题，
  // 但是如果在Write过程中出现了错误，那就会有问题了
  AsyncSocketImpl::Ptr live_this = shared_from_this();
  SignalSocketReadEvent(live_this, buffer);
}

void AsyncSocketImpl::SocketWriteComplete() {
  // 确保生命周期内不会因为外部删除而删除了整个对象
  // 如果是Socket Read事件导致的Socket错误，并不会出问题，
  // 但是如果在Write过程中出现了错误，那就会有问题了
  AsyncSocketImpl::Ptr live_this = shared_from_this();
  SignalSocketWriteEvent(live_this);
}

int AsyncSocketImpl::GetBlockCnt() {
  int res = 0;
  if (write_buffers_) {
    res += write_buffers_->blocks().size();
  }
  if (send_buffers_) {
    res += send_buffers_->blocks().size();
  }
  return res;
}

int AsyncSocketImpl::GetBlocksSize() {
  int res = 0;
  if (write_buffers_) {
    res += (int)write_buffers_->Length();
  }
  if (send_buffers_) {
    res += (int)send_buffers_->Length();
  }
  return res;
}

int AsyncSocketImpl::GetTotalBlockCnt() {
  return blocks_count_;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

AsyncListenerImpl::AsyncListenerImpl(EventService::Ptr es)
  : event_service_(es) {
}

AsyncListenerImpl::~AsyncListenerImpl() {
  DLOG_WARNING(MOD_EB, "Destory AsyncListenerImpl");
  Close();
}

bool AsyncListenerImpl::Start(const SocketAddress addr, bool addr_reused) {
  ASSERT_RETURN_FAILURE(addr.IsNil(), false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_, false);
  ASSERT_RETURN_FAILURE(accept_event_, false);

  socket_ = event_service_->CreateSocket(TCP_SOCKET);
  ASSERT_RETURN_FAILURE(!socket_, false);

  accept_event_ = event_service_->CreateDispEvent(socket_, DE_ACCEPT);
  ASSERT_RETURN_FAILURE(!accept_event_, false);
  accept_event_->SignalEvent.connect(this, &AsyncListenerImpl::OnAcceptEvent);

  // Set socket reuse option
  if (addr_reused) {
    socket_->SetOption(OPT_REUSEADDR, 1);
  }
  if (socket_->Bind(addr) == SOCKET_ERROR) {
    DLOG_ERROR(MOD_EB, "Failure to bind local address:%s",
               addr.ToString().c_str());
    return false;
  }
  DLOG_INFO(MOD_EB, "Bind:%s", addr.ToString().c_str());
  ASSERT_RETURN_FAILURE(socket_->Listen(10) == SOCKET_ERROR, false);
  listen_address_ = addr;
  return event_service_->Add(accept_event_);
}

void AsyncListenerImpl::Close() {
  if (accept_event_) {
    event_service_->Remove(accept_event_);
    accept_event_->Close();
    accept_event_.reset();
  }
  if (socket_) {
    socket_->Close();
    socket_.reset();
  }
}

const SocketAddress AsyncListenerImpl::BindAddress() {
  return listen_address_;
}

void AsyncListenerImpl::OnAcceptEvent(EventDispatcher::Ptr accept_event,
                                      Socket::Ptr socket,
                                      uint32 event_type,
                                      int err) {
  if (err || (event_type & DE_CLOSE)) {
    DLOG_ERROR(MOD_EB, "Accept event error");
    SignalNewConnected(shared_from_this(), Socket::Ptr(), err);
    return ;
  }
  if (event_type & DE_ACCEPT) {
    SignalNewConnected(shared_from_this(), socket->Accept(NULL), err);
    event_service_->Add(accept_event_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AsyncConnecterImpl::AsyncConnecterImpl(EventService::Ptr es)
  : event_service_(es) {
}

AsyncConnecterImpl::~AsyncConnecterImpl() {
  DLOG_INFO(MOD_EB, "Destory AsyncConnecterImpl");
  Close();
}

bool AsyncConnecterImpl::Connect(const SocketAddress addr, uint32 time_out) {
  ASSERT_RETURN_FAILURE(addr.IsNil(), false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_, false);
  ASSERT_RETURN_FAILURE(connect_event_, false);

  socket_ = event_service_->CreateSocket(TCP_SOCKET);
  ASSERT_RETURN_FAILURE(!socket_, false);

  connect_event_ = event_service_->CreateDispEvent(socket_, DE_CONNECT);
  ASSERT_RETURN_FAILURE(!connect_event_, false);
  connect_event_->SignalEvent.connect(
    this, &AsyncConnecterImpl::OnConnectEvent);

  // Set socket reuse option
  if (socket_->Connect(addr) == SOCKET_ERROR) {
    DLOG_ERROR(MOD_EB, "Failure to connect address:%s", addr.ToString().c_str());
    return false;
  }
  connect_address_ = addr;
  return event_service_->Add(connect_event_);

  return true;
}

void AsyncConnecterImpl::Close() {
  if (connect_event_) {
    event_service_->Remove(connect_event_);
    connect_event_->Close();
    connect_event_.reset();
  }
  if (socket_) {
    //socket_->Close();
    socket_.reset();
  }
}

const SocketAddress AsyncConnecterImpl::ConnectAddress() {
  return connect_address_;
}

////////////////////////////////////////////////////////////////////////////////
void AsyncConnecterImpl::OnConnectEvent(EventDispatcher::Ptr accept_event,
                                        Socket::Ptr socket,
                                        uint32 event_type,
                                        int err) {
  if (err || (event_type & DE_CLOSE)) {
    DLOG_ERROR(MOD_EB, "Accept event error");
    AsyncConnecterImpl::Ptr live_this = shared_from_this();
    SignalServerConnected(live_this, Socket::Ptr(), err);
    return ;
  }
  if (event_type & DE_CONNECT) {
    SignalServerConnected(shared_from_this(), socket, err);
  }
}

}  // namespace vzes
