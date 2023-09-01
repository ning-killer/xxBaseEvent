//


#ifndef EVENTSERVICES_BASE_THREAD_H_
#define EVENTSERVICES_BASE_THREAD_H_

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#ifdef POSIX
#include <pthread.h>
#endif

#include "eventservice/base/constructormagic.h"
#include "eventservice/event/messagequeue.h"

#ifdef WIN32
#include "eventservice/base/win32.h"
#endif

#include "eventservice/tls/tls.h"

namespace vzes {

class Thread;

// thread管理类定义
class ThreadManager {
 public:
  ThreadManager();
  ~ThreadManager();

  // 创建Thread Manager
  static ThreadManager* Instance();

  // 获得当前运行线程对象实例
  Thread* CurrentThread();

  // 设置当前运行线程对象实例
  void SetCurrentThread(Thread* thread);

  // 绑定当前运行线程到thread对象
  // Returns a thread object with its thread_ ivar set
  // to whatever the OS uses to represent the thread.
  // If there already *is* a Thread object corresponding to this thread,
  // this method will return that.  Otherwise it creates a new Thread
  // object whose wrapped() method will return true, and whose
  // handle will, on Win32, be opened with only synchronization privileges -
  // if you need more privilegs, rather than changing this method, please
  // write additional code to adjust the privileges, or call a different
  // factory method of your own devising, because this one gets used in
  // unexpected contexts (like inside browser plugins) and it would be a
  // shame to break it.  It is also conceivable on Win32 that we won't even
  // be able to get synchronization privileges, in which case the result
  // will have a NULL handle.
  Thread *WrapCurrentThread();

  //解绑当前运行线程thread对象
  void UnwrapCurrentThread();

 private:

  TLS_KEY key_;
  Tls     *tls_instance_;

  DISALLOW_COPY_AND_ASSIGN(ThreadManager);
};

class Thread;

// Send消息结构体
struct _SendMessage {
  _SendMessage() {}
  Thread *thread; // 发送线程对象
  Message msg;    // 消息
  bool *ready;    // 消息处理标识
};

// 线程优先级
enum ThreadPriority {
  PRIORITY_IDLE = -1,
  PRIORITY_NORMAL = 0,
  PRIORITY_ABOVE_NORMAL = 1,
  PRIORITY_HIGH = 2,
};

class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Run(Thread* thread) = 0;

 protected:
  Runnable() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Runnable);
};

// 线程初始化结构体
struct ThreadInit {
  Thread* thread;    // 线程对象
  Runnable* runnable; // runnable对象
};

// 线程类，为线程提供消息、网络事件服务
typedef boost::shared_ptr<Thread> ThreadPtr;
class Thread : public MessageQueue {
 public:
  typedef boost::shared_ptr<Thread> Ptr;
 public:
  // 创建线程
  // ss:SocketServer对象
  // name:线程名，最大长度16Byte
  // stack_size: 线程堆栈大小
  Thread(SocketServer* ss = NULL,
         const std::string &name = "vz_thread",
         const size_t stack_size = 0);
  virtual ~Thread();

  // 获取当前线程对象
  static inline Thread* Current() {
    return ThreadManager::Instance()->CurrentThread();
  }

  // 判断线程对象是不是当前运行线程对象
  bool IsCurrent() const {
    return ThreadManager::Instance()->CurrentThread() == this;
  }

  // Sleeps the calling thread for the specified number of milliseconds, during
  // which time no processing is performed. Returns false if sleeping was
  // interrupted by a signal (POSIX only).
  static bool SleepMs(int millis);

  // Sets the thread's name, for debugging. Must be called before Start().
  // If |obj| is non-NULL, its value is appended to |name|.
  const std::string& name() const {
    return name_;
  }

  // Set the name of the calling thread, Is valid only in the context
  // of the current thread.
  // The name can be up to 16 bytes long, including the terminating
  // null byte, If the length of the string, including the terminating
  // null byte, exceeds 16 bytes, the string is silently truncated.
  bool SetName(const std::string& name);

  ThreadPriority priority() const {
    return priority_;
  }

  // 设置当前线程优先级
  // priority: 线程优先级定义宏
  bool SetPriority(ThreadPriority priority);

  // 获取线程CPU亲和性(绑定CPU)
  // 结果为掩码，例如0x101代表可运行在第0,第2颗核心上
  // WIN平台, LITEOS 不支持,永远返回false;
  bool affinity(uint32 &cpu_affinity_mask);
  // 设置线程CPU亲和性(绑定CPU)
  // 结果为掩码，例如0x101代表可运行在第0,第2颗核心上
  // LITEOS 不支持，永远返回false;
  bool SetAffinity(uint32 cpu_affinity_mask);

  // Starts the execution of the thread.
  bool started() const {
    return started_;
  }
  bool Start(Runnable* runnable = NULL);

  // Used for fire-and-forget threads.  Deletes this thread object when the
  // Run method returns.
  void Release() {
    delete_self_when_complete_ = true;
  }

  // Tells the thread to stop and waits until it is joined.
  // Never call Stop on the current thread.  Instead use the inherited Quit
  // function which will exit the base MessageQueue without terminating the
  // underlying OS thread.
  virtual void Stop();

  // By default, Thread::Run() calls ProcessMessages(kForever).  To do other
  // work, override Run().  To receive and dispatch messages, call
  // ProcessMessages occasionally.
  virtual void Run();

  // 向当前线程发送同步消息
  // phandler: MessageHandler对象
  // id: 消息ID
  // pdata: 消息数据
  virtual void Send(MessageHandler *phandler, uint32 id = 0,
                    MessageData::Ptr pdata = MessageData::Ptr());

  // 清空指定MessageHandler的所有未处理消息：send、post、postdelay消息
  // phandler: MessageHandler对象
  // id: 指定消息ID，MQID_ANY表示清空所有的消息
  // removed: 输出，被清除消息list
  virtual void Clear(MessageHandler *phandler, uint32 id = MQID_ANY,
                     MessageList* removed = NULL);

  // 处理Send消息
  virtual void ReceiveSends();

  // ProcessMessages will process I/O and dispatch messages until:
  //  1) cms milliseconds have elapsed (returns true)
  //  2) Stop() is called (returns false)
  bool ProcessMessages(int cms);

  // Returns true if this is a thread that we created using the standard
  // constructor, false if it was created by a call to
  // ThreadManager::WrapCurrentThread().  The main thread of an application
  // is generally not owned, since the OS representation of the thread
  // obviously exists before we can get to it.
  // You cannot call Start on non-owned threads.
  bool IsOwned();

#ifdef WIN32
  HANDLE GetHandle() const {
    return thread_;
  }
  DWORD GetId() const {
    return thread_id_;
  }
#elif POSIX
  pthread_t GetPThread() {
    return thread_;
  }
#endif

  // This method should be called when thread is created using non standard
  // method, like derived implementation of vzes::Thread and it can not be
  // started by calling Start(). This will set started flag to true and
  // owned to false. This must be called from the current thread.
  // NOTE: These methods should be used by the derived classes only, added here
  // only for testing.
  bool WrapCurrent();

  // 解绑当前线程
  void UnwrapCurrent();

 protected:
  // Blocks the calling thread until this thread has terminated.
  void Join();

 private:
  static void *PreRun(void *pv);
  static ThreadInit *CreateThreadInit();
  static void DestoryThreadInit(ThreadInit *init);

  // ThreadManager calls this instead WrapCurrent() because
  // ThreadManager::Instance() cannot be used while ThreadManager is
  // being created.
  bool WrapCurrentWithThreadManager(ThreadManager* thread_manager);

  std::list<_SendMessage> sendlist_; // Send消息列表
  std::string name_;                // 线程名
  size_t stack_size_;              // 线程堆栈大小
  ThreadPriority priority_;         // 线程优先级
  bool started_;                   // 线程启动标志
  bool has_sends_;                 // 是否有Send消息标志

#ifdef POSIX
  pthread_t thread_;
#endif

#ifdef WIN32
  HANDLE thread_;
  DWORD thread_id_;
#endif

  bool owned_;
  bool delete_self_when_complete_;

  friend class ThreadManager;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

// AutoThread automatically installs itself at construction
// uninstalls at destruction, if a Thread object is
// _not already_ associated with the current OS thread.

class AutoThread : public Thread {
 public:
  AutoThread(SocketServer* ss = 0);
  virtual ~AutoThread();

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoThread);
};

template <class T>
class ThreadEx : public vzes::Thread, public vzes::Runnable {
public:
  typedef boost::shared_ptr<ThreadEx> Ptr;
  ThreadEx(T *context = NULL, const std::string &name = "", const size_t stack_size = 0)
      : Thread(NULL, name, stack_size), context_(context), need_stop_(false) {}
  ~ThreadEx() override = default;
  virtual bool Start() {
    need_stop_ = false;
    return Thread::Start(this);
  };
  void Stop() override {
    need_stop_ = true;
    Thread::Stop();
  }
  void Run(vzes::Thread *thread) override {
    // 绑定到CPU0运行
    SetAffinity((1 << 0));
    Run(context_);
  };
  virtual void Run(T *context) = 0;

  T *context_;
  bool need_stop_;
};

// Win32 extension for threads that need to use COM
#ifdef WIN32

// 该类是一个简单的辅助工具类
// 帮助在Windows平台下需要使用COM组件的线程在运行的时候调用CoInitializeEx接口
// 在结束的时候调用CoUninitialize接口
class ComThread : public Thread {
 public:
  ComThread() {}

 protected:
  virtual void Run();

 private:
  DISALLOW_COPY_AND_ASSIGN(ComThread);
};
#endif

// Provides an easy way to install/uninstall a socketserver on a thread.
class SocketServerScope {
 public:
  explicit SocketServerScope(SocketServer* ss) {
    old_ss_ = Thread::Current()->socketserver();
    Thread::Current()->set_socketserver(ss);
  }
  ~SocketServerScope() {
    Thread::Current()->set_socketserver(old_ss_);
  }

 private:
  SocketServer* old_ss_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SocketServerScope);
};

}  // namespace vzes

#endif  // EVENTSERVICES_BASE_THREAD_H_
