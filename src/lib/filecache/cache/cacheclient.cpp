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

#include "filecache/cache/cacheclient.h"
#include "filecache/server/cacheserver.h"

namespace cache {

class CacheClientImpl : public CacheClient,
  public boost::noncopyable,
  public boost::enable_shared_from_this<CacheClientImpl>,
  public sigslot::has_slots<> {
 public:
  CacheClientImpl() {
    cache_service_ = CacheServer::Instance();
    ASSERT(cache_service_ != NULL);
    use_absolute_path_ = false;
  }

  virtual ~CacheClientImpl() {
  }

  virtual bool Write(const char *file_name, const char *data, int data_size,
                     char path[128]) {
    // 不需要加锁，各层保证各自支持多线程:client, server, service
    //vzes::CritScope cr(&crit_);
    if (NULL == file_name) {
      DLOG_ERROR(MOD_EB, "write file failed, nil file name");
	  return CACHED_FAILURE;
	}
	if ((NULL == data) || (FILE_MIN_SIZE > data_size)) {
      DLOG_ERROR(MOD_EB, "write file %s failed, invalid data(ptr:%p, size:%d)",
	  	         file_name, data, data_size);
	  return CACHED_FAILURE;
	}
	
    DLOG_INFO(MOD_EB, "write file request:%s,size:%d, last byte:%X",
              file_name, data_size,
              (int)(unsigned char)data[data_size - 1]);
    CacheData::Ptr cache_data(new CacheData());
    cache_data->buffer->WriteBytes(data, data_size);
    cache_data->path.append(file_name, strlen(file_name));
    cache_data->absolute_path = use_absolute_path_;
    cache_data->res = CACHE_ERROR_TIMEOUT;
    cache_service_->AsyncWrite(cache_data);
    if (CACHE_DONE != cache_data->res) {
      return CACHED_FAILURE;
    }

    if (!use_absolute_path_) {
      memcpy(path, cache_data->out_path.c_str(), cache_data->out_path.size());
      path[cache_data->out_path.size()] = '\0';
    }
    return CACHED_SUCCEED;
  }

  virtual bool Write(const char *file_name, vzes::MemBuffer::Ptr data,
                     char path[128]) {
    // 不需要加锁，各层保证各自支持多线程:client, server, service
    //vzes::CritScope cr(&crit_);
    if (NULL == file_name) {
      DLOG_ERROR(MOD_EB, "write file failed, nil file name");
	  return CACHED_FAILURE;
	}
	if (NULL == data.get()) {
      DLOG_ERROR(MOD_EB, "write file %s failed, nil data buffer", file_name);
	  return CACHED_FAILURE;
	}
	if (FILE_MIN_SIZE > data->size()) {
      DLOG_ERROR(MOD_EB, "write file %s failed, invlid data size %d", data->size());
	  return CACHED_FAILURE;
	}

    vzes::BlocksPtr &block_list = data->blocks();
    vzes::Block::Ptr block = block_list.back();
    DLOG_INFO(MOD_EB, "write file request:%s,size:%d, last byte:%X",
              path, data->size(),
              (int)(unsigned char)block->buffer[block->buffer_size - 1]);
    CacheData::Ptr cache_data(new CacheData());
    data->CopyBuffer(cache_data->buffer, data->size());
    cache_data->path.append(file_name, strlen(file_name));
    cache_data->absolute_path = use_absolute_path_;
    cache_data->res = CACHE_ERROR_TIMEOUT;
    cache_service_->AsyncWrite(cache_data);
    if (CACHE_DONE != cache_data->res) {
      return CACHED_FAILURE;
    }

    if (!use_absolute_path_) {
      memcpy(path, cache_data->out_path.c_str(), cache_data->out_path.size());
      path[cache_data->out_path.size()] = '\0';
    }
    return CACHED_SUCCEED;
  }

  virtual MemBuffer::Ptr Read(const char *path) {
    //vzes::CritScope cr(&crit_);
    if (NULL == path) {
      DLOG_ERROR(MOD_EB, "read file failed, invalid path:%p", path);
      return MemBuffer::Ptr();
    }
    if (0 == strlen(path)) {
      DLOG_ERROR(MOD_EB, "read file failed, path length: 0");
      return MemBuffer::Ptr();
    }
    DLOG_INFO(MOD_EB, "read file request:%s\n", path);
    CacheData::Ptr cache_data(new CacheData());
    cache_data->buffer = MemBuffer::Ptr();
    cache_data->path.append(path, strlen(path));
    cache_data->res = CACHE_ERROR_TIMEOUT;

    cache_service_->SyncRead(cache_data);
    if (CACHE_DONE != cache_data->res) {
      return MemBuffer::Ptr();
    }
    MemBuffer::Ptr tmp = MemBuffer::CreateMemBuffer();
    cache_data->buffer->CopyBuffer(tmp, cache_data->buffer->size());
    return tmp;
  }

  virtual bool Delete(const char *path) {
    //vzes::CritScope cr(&crit_);
    if (NULL == path) {
      DLOG_ERROR(MOD_EB, "Delete file failed,invalid paras(path:%p)", path);
      return CACHED_FAILURE;
    }
    CacheData::Ptr cache_data(new CacheData());
    cache_data->path.append(path, strlen(path));
    cache_data->res = CACHE_ERROR_TIMEOUT;

    cache_service_->AsyncDelete(cache_data);
    return CACHED_SUCCEED;
  }

  virtual void ReleaseCache() {
    CacheData::Ptr cache_data(new CacheData());
    cache_data->res = CACHE_ERROR_TIMEOUT;
    cache_service_->ReleaseCache(cache_data);
  }

  virtual void SetPathMode(bool use_abs_path) {
    use_absolute_path_ = use_abs_path;
    DLOG_INFO(MOD_EB, "Set path mode, use_abs_path: %d", use_abs_path);
  }

 private:
  CacheServer            *cache_service_;
  vzes::CriticalSection   crit_;
  bool                    use_absolute_path_;
};

CacheClient::Ptr CacheClient::CreateCacheClient() {
  CacheClient::Ptr cc(new CacheClientImpl());
  return cc;
}

void CacheClient::DumpCacheInfo() {
  CacheServer *cache_srv = CacheServer::Instance();
  cache_srv->DumpCacheInfo();
}

}

