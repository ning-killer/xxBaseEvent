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


#ifndef EVENTSERVICE_EVENT_SIGNALEVENT_H_
#define EVENTSERVICE_EVENT_SIGNALEVENT_H_

#include "eventservice/base/basicincludes.h"

namespace vzes {

//事件处理结果
#define SIGNAL_EVENT_FAILURE    2   // 失败
#define SIGNAL_EVENT_TIMEOUT    1   // 超时
#define SIGNAL_EVENT_DONE       0   // 成功

#ifdef POSIX
typedef int SOCKET;
#endif // POSIX

// Event constants for the Dispatcher class.
//socket事件类型枚举体
enum DispatcherEvent {
  DE_READ = 0x0001,     // 读   00001
  DE_WRITE = 0x0002,    // 写   00010
  DE_CONNECT = 0x0004,  // 连接 00100
  DE_CLOSE = 0x0008,    // 关闭 01000
  DE_ACCEPT = 0x0010,   // 接受 10000 
};

// signal类定义，用于线程间同步
class SignalEvent {
 public:
  typedef boost::shared_ptr<SignalEvent> Ptr;

  // 创建signal对象
  static SignalEvent::Ptr CreateSignalEvent();
  virtual ~SignalEvent() {};

  // 等待信号
  // wait_millsecond: 超时时长(毫秒)
  // return 失败 SIGNAL_EVENT_FAILURE；超时 SIGNAL_EVENT_TIMEOUT；
  // 成功 SIGNAL_EVENT_DONE
  virtual int WaitSignal(uint32 wait_millsecond) = 0;

  // 判断信号是否超时
  virtual bool IsTimeout() = 0;

  // 重置signal
  virtual void ResetTriggerSignal() = 0;

  // 触发signal
  virtual void TriggerSignal() = 0;
};

// linux signal实现类定义
class Signal {
 public:

   // 创建signal对象
  static Signal *CreateSignal();
  virtual ~Signal() {};

  // 重置signal
  virtual void ResetWakeEvent() = 0;

  // 触发signal
  virtual void SignalWakeup() = 0;

  // 获取信号对象FD
  virtual SOCKET GetSocket() = 0;

  // 获取信号对象事件
  virtual uint32 GetEvents() const = 0;
};

} // namespace vzes

#endif // EVENTSERVICE_EVENT_SIGNALEVENT_H_
