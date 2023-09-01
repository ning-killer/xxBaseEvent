//
#include "log/log/log_client.h"
#include "filecache/server/cacheserver.h"
#include "eventservice/base/common.h"
#include <stdio.h>

namespace cache {

#define TYPE_ASYNC_DELETE     1
#define TYPE_ASYNC_WRITE      2
#define TYPE_ASYNC_READ       3

#define TYPE_CACHE_GATE       4

#define TYPE_KVDB_SET_KEY     5
#define TYPE_KVDB_GET_KEY     6
#define TYPE_KVDB_DELETE_KEY  7
#define TYPE_KVDB_BACKUP      8
#define TYPE_KVDB_RESTORE     9

#define TYPE_KVDB_GATE        10

#define CACHE_PATH_BUFF_LEN   512

unsigned int g_cache_data_count = 0;
CacheServer *CacheServer::instance_ = NULL;

CacheServer *CacheServer::Instance() {
  if (!instance_) {
    instance_ = new CacheServer();
#ifndef NO_FILE_CACHE
    instance_->InitCacheService();
#endif
  }
  return instance_;
}

CacheServer::CacheServer() {
}

CacheServer::~CacheServer() {
  event_service_->UninitEventService();
  KVDBs::iterator iter;
  for (iter = kvdbs_.begin(); iter != kvdbs_.end(); ++iter) {
    delete iter->second;
  }
  kvdbs_.clear();
}

void CacheServer::InitCacheService() {
  event_service_ = vzes::EventService::CreateEventService(NULL, "vz_cacheSrv");
  event_service_->SetThreadPriority(vzes::PRIORITY_IDLE);
  cache_service_ = CachedService::Ptr(new CachedService(event_service_));
}

/* 同步 */
void CacheServer::SyncRead(CacheData::Ptr cache_data) {
  cache_data->buffer = cache_service_->GetFile(cache_data->path);
  if (cache_data->buffer) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  }
}

/* 异步 */
void CacheServer::AsyncWrite(CacheData::Ptr cache_data) {
  bool res = false;
  if (cache_data->absolute_path) {
    res = cache_service_->SaveFile(cache_data->path, cache_data->buffer);
  } else {
    res = cache_service_->SaveFile(cache_data->path, cache_data->buffer,
                                   cache_data->out_path);
  }

  if (res) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_NO_MEM;
  }
}

/* 异步 */
void CacheServer::AsyncDelete(CacheData::Ptr cache_data) {
  event_service_->Post(this, TYPE_ASYNC_DELETE, cache_data);
}

void CacheServer::ReleaseCache(CacheData::Ptr cache_data) {
  cache_service_->ReleaseCache();
  cache_data->res = CACHE_DONE;
}

void CacheServer::DumpCacheInfo() {
  cache_service_->DumpCacheInfo();
}

void CacheServer::OnMessage(vzes::Message *msg) {
  ASSERT(event_service_->IsThisThread(vzes::Thread::Current()));

  if (msg->message_id < TYPE_CACHE_GATE) {
    CacheData::Ptr cache_data =
      boost::dynamic_pointer_cast<CacheData>(msg->pdata);
    if (msg->message_id == TYPE_ASYNC_DELETE) {
      OnDeleteEvent(cache_data);
    } else {
      DLOG_ERROR(MOD_EB, "Unkown CACHE message id %d", msg->message_id);
    }

  } else {
    DLOG_ERROR(MOD_EB, "Unkown CACHE message id %d", msg->message_id);
  }
}

void CacheServer::OnDeleteEvent(CacheData::Ptr cache_data) {
  if (cache_service_->RemoveFile(cache_data->path)) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  }
}

////////////////////////////////////////////////////////////////////////////////
void CacheServer::KVDBSetKey(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p) {
    rslt = p->Replace(kvdb_data->key, kvdb_data->value);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
}

void CacheServer::KVDBGetKey(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p) {
    rslt = p->Seek(kvdb_data->key, kvdb_data->value);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
}

void CacheServer::KVDBDeleteKey(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p) {
    rslt = p->Remove(kvdb_data->key);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
}

void CacheServer::KVDBBackUp(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;
  char bak_foldpath[CACHE_PATH_BUFF_LEN];

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p) {
#ifdef WIN32
    snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    rslt = p->Backup(bak_foldpath);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
}

void CacheServer::KVDBRestore(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;
  char foldpath[CACHE_PATH_BUFF_LEN];

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p) {
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    rslt = p->Restore(foldpath);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }

    //backup main folder
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    int tmp = p->Clear(foldpath);
    if (tmp != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      rslt = tmp;
    }
  }
}

void CacheServer::KVDBClear(cache::KvdbData::Ptr kvdb_data, uint8 clear_type) {
  KvdbError rslt = KVDB_ERR_OK, tmp;
  KvdbService *p;
  char foldpath[CACHE_PATH_BUFF_LEN];

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if (p == NULL) {
    return;
  }

  if (clear_type & KVDB_CLEAR_TYPE_MAIN) {
    //main folder
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    tmp = p->Clear(foldpath);
    if (tmp != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      rslt = tmp;
    }

    //backup main folder
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    tmp = p->Clear(foldpath);
    if (tmp != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      rslt = tmp;
    }
  }

  if (clear_type & KVDB_CLEAR_TYPE_BAK) {
    //backup bak folder
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_BACKUP_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    tmp = p->Clear(foldpath);
    if (tmp != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      rslt = tmp;
    }

    //bak folder
#ifdef WIN32
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    tmp = p->Clear(foldpath);
    if (tmp != KVDB_ERR_OK) {
      DLOG_WARNING(MOD_EB, "Clear folder %s failed.", foldpath);
      rslt = tmp;
    }
  }

  if (rslt == KVDB_ERR_OK) {
    kvdb_data->res = CACHE_DONE;
  }
}

bool CacheServer::KVDBSetProperty(std::string name, int property) {
  KvdbService *p;

  p = GetKvdb(name);
  if (!p) {
    return false;
  }
  return p->SetProperty(property);
}
bool CacheServer::KVDBGetProperty(std::string name, int &property) {
  KvdbService *p;

  p = GetKvdb(name);
  if (!p) {
    return false;
  }
  return p->GetProperty(property);
}

KvdbService *CacheServer::GetKvdb(std::string &name) {
  // map操作多线程互斥
  vzes::CritScope cr(&kvdb_crit_);

  char buf[CACHE_PATH_BUFF_LEN];
  KvdbService *p = NULL;
  KVDBs::iterator iter = kvdbs_.find(name);
  if (iter != kvdbs_.end()) {
    p = iter->second;
  } else if (name.length() > 0) {
    struct KvdbCache cache[2];
    cache[0].size = 128;
    cache[0].counter = 50;  // 6KB
    cache[1].size = 2048;
    cache[1].counter = 10;  // 20KB

    MAKE_FILE_NAME(buf, CACHE_PATH_BUFF_LEN, KVDB_PARENT_FOLD, name.c_str());
    p = new KvdbService(buf, cache, 2);
    kvdbs_[name] = p;
  }
  return p;
}

}
