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
#ifndef EVENTSERVICE_DP_DPCLIENT_H_
#define EVENTSERVICE_DP_DPCLIENT_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"
#include "astl/include/string.hpp"

namespace vzes {

typedef std::string DpBuffer;

enum {
  TYPE_INVALID = -1,            // 非法TYPE
  TYPE_MESSAGE = 0,
  TYPE_REQUEST = 1,
  TYPE_REPLY = 2,
  TYPE_ERROR_TIMEOUT = 3,
  TYPE_ERROR_FORMAT = 4,
  TYPE_GET_SESSION_ID = 5,
  TYPE_SUCCEED = 6,
  TYPE_FAILURE = 7,
  TYPE_ADD_MESSAGE = 8,
  TYPE_REMOVE_MESSAGE = 9,
  TYPE_ERROR_DISCONNECTED = 10,
};

extern unsigned int g_dpmessage_count;
struct DpMessage : public MessageData {
  DpMessage() {
    g_dpmessage_count++;
  }
  ~DpMessage() {
    signal_event.reset();
    g_dpmessage_count--;
  }
  typedef boost::shared_ptr<DpMessage> Ptr;
  uint32            type;
  std::string       method;      // DP消息描述
  uint32            session_id;  // 会话id
  DpBuffer          req_buffer;  // Request数据，Request端填写
  DpBuffer         *res_buffer;  // Replay数据，Replay端填写
  bool              isTimeOut;   // Replay超时标识
  DpBuffer          tmp_res;     // 临时变量，兼容之前的用法
  SignalEvent::Ptr  signal_event;
};

typedef struct {
  uint8             type;        // 消息类型
  uint8             method_size; // Dp method长度
  uint32            timeout;     // DP Request超时时间(单位：ms)
  uint32            message_id;  // 消息ID
  uint32            session_id;  // DP消息Session ID
  uint32            data_size;   // DP消息用户数据长度
  char              body[0];     // 数据：method + data
} DpNetMsgHeader;

class DpClient {
 public:
  typedef boost::shared_ptr<DpClient> Ptr;
  // 回调函数，请求、广播消息回调函数
  sigslot::signal2<DpClient::Ptr, DpMessage::Ptr> SignalDpMessage;
  // 回调函数，错误事件回调函数
  sigslot::signal2<DpClient::Ptr, int> SignalErrorEvent;
  virtual ~DpClient() {};

  // 创建一个本地DpClient对象
  // es:EventService，指定处理DP消息时运行的线程上下文
  // return 成功，！= NULL; 失败，NULL
  static DpClient::Ptr CreateDpClient(EventService::Ptr es);

  // 删除一个DpClient对象，程序退出的时候，应该删除这个对象
  // dp_client: DpClient对象
  static void DestoryDpClient(DpClient::Ptr dp_client);

  // 创建一个远端DpClient对象，链接至Server端星型结构
  // es:EventService，指定处理DP消息时运行的线程上下文
  // return 成功，！= NULL; 失败，NULL
  static DpClient::Ptr CreateDpNetClient(EventService::Ptr es,
                                         const SocketAddress addr);

  // 删除一个DpClient对象，程序退出的时候，应该删除这个对象
  // dp_client: DpClient对象
  static void DestoryDpNetClient(DpClient::Ptr dp_client);

  // 发送一个广播消息，消息数据和内容由自己指定
  // method: 消息描述
  // session_id: 会话id，用户自定义
  // data: 消息内容
  // data_size: 消息内容长度（Byte）
  // return 成功，true；失败，false
  virtual bool SendDpMessage(const std::string method, uint32 session_id,
                             const char *data, uint32 data_size) = 0;

  // 发送一个星形结构请求，请求的结果通过指针res_buffer返回，
  // 该接口为阻塞调用，直到收到回复消息或者超时时返回
  // method: 消息描述
  // session_id：会话id，用户自定义
  // data: 请求消息内容
  // data_size: 消息内容长度（Byte）
  // res_buffer: 输出参数，由Request端提供内存，Replay端填写回复消息内容
  // timeout_millisecond: 超时时常（ms）
  // return 成功，true；失败，false
  virtual bool SendDpRequest(const std::string method, uint32 session_id,
                             const char *data, uint32 data_size,
                             DpBuffer *res_buffer, uint32 timeout) = 0;

  // 回复一个星形结构消息，收到DP请求后，直接填写回复内容到请求
  // DpMessage中的res_buffer, res_buffer内存由request端提供
  // dp_msg：dp消息指针，即DP Request收到的dp消息指针
  // return 成功，true；失败，false
  virtual bool SendDpReply(DpMessage::Ptr dp_msg) = 0;

  // 注册一个监听消息
  // method：dp 消息描述
  virtual void ListenMessage(const std::string method) = 0;

  // 取消一个消息监听
  // method：dp 消息描述
  virtual void RemoveMessage(const std::string method) = 0;
};

}

#endif  // EVENTSERVICE_DP_DPCLIENT_H_

