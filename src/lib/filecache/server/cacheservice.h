//

#ifndef FILE_CACHE_CONN_CACHED_SERVICES_H_
#define FILE_CACHE_CONN_CACHED_SERVICES_H_

#include <set>
#include <deque>
#include <vector>
#include "eventservice/base/noncopyable.h"
#include "eventservice/net/eventservice.h"
#include "filecache/server/cachestanzapool.h"
#include "filecache/base/basedefine.h"
#include "filecache/base/diskhelper.h"


namespace cache {

// Filecache文件夹监控条目信息
struct FlcStanza {
  uint32 max_size;
  std::string path;
  FlcStanza &operator=(const FlcStanza &stanza) {
    max_size = stanza.max_size;
    path     = stanza.path;
    return *this;
  }
};

// Cache Server线程消息定义
enum CachedMessage {
  CACHED_ADD,         // 写文件到flash
  CACHED_CHECK,       // 检查Filecache文件夹大小
  CACHED_FORMAT_PART, // 格式化磁盘分区
  CACHED_RENICE       // 调整服务线程nice值
};

class CachedService : public vzes::noncopyable,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<CachedService> Ptr;

 public:
  CachedService(vzes::EventService::Ptr es);
  virtual ~CachedService();

  // 存储文件，用户自定义相对路径，绝对路径由service层生成
  // path:文件相对路径
  // data:文件数据
  // abs_path_name:输出参数，绝对路径
  bool SaveFile(const std::string file_name, MemBuffer::Ptr data,
                std::string &abs_path_name);
  // 存储文件
  // path:文件绝对路径，全路径+文件名
  // data:文件数据
  bool SaveFile(const std::string file_name, MemBuffer::Ptr data);
  // 读取文件
  // path:文件路径，全路径+文件名
  // return 文件数据Membuffer指针
  MemBuffer::Ptr GetFile(const std::string path);
  // 删除文件
  // path:文件路径，全路径+文件名
  bool RemoveFile(const std::string path);
  // 释放当前没有被使用的缓存
  void ReleaseCache();
  void DumpCacheInfo();

 private:
  virtual void OnMessage(vzes::Message *msg);

 private:
  bool Start();
  uint64 GetFolderSize(std::string folder_path);
  bool RemoveFolderFiles(std::string folder_path);
  void CheckFileLimit();
  bool InitFileLimitCheck();
  void StartFileLimitCheckTimer();
  bool IsStanzaFree(CachedStanza::Ptr stanza);
  void RemoveOutOfDataStanza(bool remove_all);
  void InitPartition();
  int  FormatPartition(uint32 device, uint32 part);
  void CheckPartitionSize();
  bool CheckSDCardStatus(bool *old_stat);

 private:
  void OnRenice();
  int  OnSaveFile(CachedStanza::Ptr stanza);
  int  AsyncSaveFile(CachedStanza::Ptr stanza);
  void OnAsyncRemoveFile(std::string path);
  void ReplaceCachedFile(CachedStanza::Ptr stanza);
  bool AddFile(CachedStanza::Ptr stanza, bool is_cached = true);
  bool ReadFile(const std::string path, MemBuffer::Ptr data_buffer);
  void MakeDirRecursive(const char *pPath);

  void GenFilePath(const char *name, char path[128]);
  void GetABSFilePath(const char *filename, char path[128]);

 private:
  struct StanzaMessageData : public vzes::MessageData {
    CachedStanza::Ptr stanza;
  };

  struct FormatDeviceMessage : public vzes::MessageData {
    uint32  device_idx;     // 设备index，flash:0, sd card：1
    uint32  partition_idx_; // 分区index
  };

 private:
  std::size_t                    cache_size_;
  vzes::CriticalSection          crit_;
  vzes::EventService::Ptr        event_service_;
  
  std::deque<CachedStanza::Ptr>  cached_stanzas_;
  
  CachedStanzaPool              *cachedstanza_pool_;
  std::vector<FlcStanza>         flc_stanzas_;

  vzes::CriticalSection          disk_crit_;  // 避免格式化和写同时进行
  uint32                         curr_part_idx_;
  bool                           sd_card_mounted_;

  vzes::DiskHelper              *disk_helper_;
};
}

#endif // FILE_CACHE_CONN_CACHED_SERVICES_H_
