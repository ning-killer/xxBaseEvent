//


#ifndef FILECACHE_SERVER_CACHEDSTANZAPOOL_H_
#define FILECACHE_SERVER_CACHEDSTANZAPOOL_H_

#include <stdio.h>
#include <list>
#include <vector>
#include <string>

#include "astl/include/string.hpp"
#include "boost/boost_settings.hpp"
#include "eventservice/base/basictypes.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/base/noncopyable.h"
#include "eventservice/base/queue.h"
#include "filecache/base/basedefine.h"

namespace cache {

class CachedStanza : public vzes::noncopyable,
  public boost::enable_shared_from_this<CachedStanza> {
 public:
  typedef boost::shared_ptr<CachedStanza> Ptr;

  CachedStanza();
  virtual ~CachedStanza();

  std::string &path() {
    return path_;
  }

  void SetPath(const char *path) {
    path_ = path;
  }

  MemBuffer::Ptr data() {
    return cache_data_;
  }

  void SetData(MemBuffer::Ptr data);

  bool IsSaved();
  void SaveConfimation();
  void ResetDefualtState();

  std::size_t size() {
    if (cache_data_) {
      return cache_data_->size();
    }

    return 0;
  }

  static uint32 stanza_count;
 private:
  std::string path_;
  MemBuffer::Ptr cache_data_;
  bool is_saved_;

  vzes::CriticalSection stanza_mutex_;
};

class CachedStanzaPool : public vzes::noncopyable,
  public boost::enable_shared_from_this<CachedStanzaPool> {
 public:
  typedef boost::shared_ptr<CachedStanzaPool> Ptr;
  CachedStanzaPool();
  virtual ~CachedStanzaPool();
  static CachedStanzaPool *Instance();
  // Thread safed
  CachedStanza::Ptr TakeStanza();
  // Thread safed
  void SetDefaultCachedSize(std::size_t stanza_size);
  std::size_t CachedStanzaSize() {
    return Queue_Size(stanza_queue_);
  }
 private:
  static void RecyleBuffer(void *stanza);
  void RecyleStanza(CachedStanza *stanza);
 private:
  QUE_HANDLE stanza_queue_;
 private:
  static CachedStanzaPool *pool_instance_;
};
}

#endif // FILECACHE_SERVER_CACHEDSTANZAPOOL_H_
