//

#include "filecache/server/cacheserver.h"
#include "filecache/base/basedefine.h"
#include "filecache/kvdb/kvdbclient.h"

namespace cache {

class KvdbClientImpl : public KvdbClient,
  public boost::noncopyable,
  public boost::enable_shared_from_this<KvdbClientImpl>,
  public sigslot::has_slots<> {
 public:
  KvdbClientImpl(const std::string& name) {
    cache_service_ = CacheServer::Instance();
    ASSERT(cache_service_ != NULL);
    name_ = name;
  }

  virtual ~KvdbClientImpl() {
  }

  bool SetKey(const char *key, uint8 key_size,
              const char *value, uint32 value_size) {
    std::string skey(key, key_size);
    return SetKey(skey, value, value_size);
  }

  bool SetKey(const std::string key, const char *value, uint32 value_size) {
    // ����Ҫ���������㱣֤����֧�ֶ��߳�:client, server, service
    //vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->key  = key;
    kvdb_data->value.append(value, value_size);
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBSetKey(kvdb_data);
    if (CACHE_DONE != kvdb_data->res) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool GetKey(const char *key, uint8 key_size, std::string *result) {
    std::string skey(key, key_size);
    return GetKey(skey, result);
  }

  bool GetKey(const std::string key, std::string *result)  {
    //vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->key  = key;
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBGetKey(kvdb_data);
    if (CACHE_DONE != kvdb_data->res) {
      return KVDB_FAILURE;
    }
    *result = kvdb_data->value;
    return KVDB_SUCCEED;
  }

  bool GetKey(const std::string key, void *buffer, std::size_t buffer_size) {
    std::string result;
    bool res = GetKey(key, &result);
    if (res != KVDB_SUCCEED) {
      return res;
    }
    if (result.size() > buffer_size) {
      DLOG_ERROR(MOD_EB, "dst buffer unenough! value len:%d, dst buffer len:%d",
                 result.size(), buffer_size);
      return KVDB_FAILURE;
    }
    memcpy(buffer, result.c_str(), result.size());
    return KVDB_SUCCEED;
  }

  bool GetKey(const char *key, uint8 key_size, void *buffer,
              std::size_t buffer_size) {
    std::string skey(key, key_size);
    return GetKey(skey, buffer, buffer_size);
  }

  bool DeleteKey(const char *key, uint8 key_size) {
    //vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->key.append(key, key_size);
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBDeleteKey(kvdb_data);
    if (CACHE_DONE != kvdb_data->res) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool SetProperty(int property) {
    return cache_service_->KVDBSetProperty(name_, property);
  }

  bool GetProperty(int &property) {
    return cache_service_->KVDBGetProperty(name_, property);
  }

  bool BackupDatabase() {
    //vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBBackUp(kvdb_data);
    if (CACHE_DONE != kvdb_data->res) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool RestoreDatabase() {
    //vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBRestore(kvdb_data);
    if (CACHE_DONE != kvdb_data->res) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool Clear(uint8 clear_type) {
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name = name_;
    kvdb_data->res  = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBClear(kvdb_data, clear_type);
    if (CACHE_DONE != kvdb_data->res) {
      DLOG_WARNING(MOD_EB, "Clear kvdb(%s) Failed kvdb_data->res = %d",
                   name_.c_str(), kvdb_data->res);
      return KVDB_FAILURE;
    }
    DLOG_INFO(MOD_EB, "Clear kvdb(%s) Succeed.",
              name_.c_str());
    return KVDB_SUCCEED;
  }

 private:
  vzes::EventService::Ptr cs_service_;
  CacheServer            *cache_service_;
  vzes::CriticalSection   crit_;
  static const uint32     DEFAULT_TIMEOUT = 5000;
  std::string             name_;
};

KvdbClient::Ptr KvdbClient::CreateKvdbClient(const std::string& name) {
  return KvdbClient::Ptr(new KvdbClientImpl(name));
}

}

