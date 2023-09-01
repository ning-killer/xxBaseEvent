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

#ifndef FILECHACHE_SERVER_CACHESERVER_H_
#define FILECHACHE_SERVER_CACHESERVER_H_

#include <string>
#include <map>

#include "astl/include/string.hpp"

#include "filecache/base/basedefine.h"
#include "filecache/server/cacheservice.h"
#include "eventservice/event/signalevent.h"
#include "eventservice/net/eventservice.h"
#include "filecache/server/kvdbservice.h"
#include "filecache/kvdb/kvdbclient.h"

namespace cache {

extern unsigned int g_cache_data_count;
// filecache消息结构体，用于Client和Server之间通信
struct CacheData : public vzes::MessageData {
  typedef boost::shared_ptr<CacheData> Ptr;
  CacheData() {
    g_cache_data_count++;
    buffer = MemBuffer::CreateMemBuffer();
  }
  ~CacheData() {
    buffer.reset();
    g_cache_data_count--;
  }
  std::string   path;      // file name
  std::string   out_path; // file full directory
  bool            absolute_path;
  MemBuffer::Ptr  buffer;    // file content buffer for read\write operations
  int             res;       // 响应结果
};

// kvdb消息结构体，用于Client和Server之间通信
struct KvdbData : public vzes::MessageData {
  KvdbData() {
  }
  ~KvdbData() {
  }
  typedef boost::shared_ptr<KvdbData> Ptr;
  std::string       name;   // KvdbClient name
  std::string       key;    // key
  std::string     value;  // value
  int               res;    // 响应结果
};


class CacheServer : public boost::noncopyable,
  public boost::enable_shared_from_this<CacheServer>,
  public sigslot::has_slots<>,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<CacheServer> Ptr;
  static CacheServer *Instance();
 public:
  virtual ~CacheServer();
  void InitCacheService();
  // Async write file, thread-safe interface
  void AsyncWrite(CacheData::Ptr cache_data);
  // Sync read file, thread-safe interface
  void SyncRead(CacheData::Ptr cache_data);
  // Async delete file, thread-safe interface
  void AsyncDelete(CacheData::Ptr cache_data);
  // Sync release unused cache
  void ReleaseCache(CacheData::Ptr cache_data);
  void DumpCacheInfo();

  // Kvdb interface, Sync & thread-safe interface
  void KVDBSetKey(KvdbData::Ptr kvdb_data);
  void KVDBGetKey(KvdbData::Ptr kvdb_data);
  void KVDBDeleteKey(KvdbData::Ptr kvdb_data);
  void KVDBBackUp(KvdbData::Ptr kvdb_data);
  void KVDBRestore(KvdbData::Ptr kvdb_data);
  void KVDBClear(KvdbData::Ptr kvdb_data,
                 uint8 clear_type = KVDB_CLEAR_TYPE_MAIN | KVDB_CLEAR_TYPE_BAK);
  bool KVDBSetProperty(std::string name, int property);
  bool KVDBGetProperty(std::string name, int &propery);
 private:
  virtual void OnMessage(vzes::Message *msg);
  //
  void OnWriteEvent(CacheData::Ptr cache_data);
  void OnReadEvent(CacheData::Ptr cache_data);
  void OnDeleteEvent(CacheData::Ptr cache_data);

  KvdbService *GetKvdb(std::string &name);
 private:
  CacheServer();
 private:
  vzes::CriticalSection  kvdb_crit_;  // kvdb operation mutex
  static CacheServer  *instance_;  // cache server single-instance object.
  CachedService::Ptr   cache_service_;  // cache service object.
  vzes::EventService::Ptr event_service_;  // Event service object.
  typedef std::map<std::string, KvdbService *> KVDBs;  // kvdb 底层操作类map
  KVDBs  kvdbs_;
};

}

#endif  // FILECHACHE_SERVER_CACHESERVER_H_
