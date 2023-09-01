//


#ifndef FILECHACHE_CACHE_CACHECLIENT_H_
#define FILECHACHE_CACHE_CACHECLIENT_H_

#include "filecache/base/basedefine.h"
#include "eventservice/net/eventservice.h"

// filecache文件夹大小监控配置文件路径,文件格式规则：
// https://note.youdao.com/share/?token=C91B3122B9824F49A3BE9D161696186E&gid=31699737
#ifdef WIN32
#define FILE_LIMIT_CONFIG      "c:\\vz_cfg\\file_cache_limit.json"
#elif defined LITEOS
#define FILE_LIMIT_CONFIG      "/usr/file_cache_limit.json"
#elif defined UBUNTU64
#define FILE_LIMIT_CONFIG      "/tmp/file_cache_limit.json"
#else
#define FILE_LIMIT_CONFIG      "/tmp/app/exec/file_cache_limit.json"
#endif

// 有效文件的最小长度(Byte)
#define FILE_MIN_SIZE          (20)

#define CACHED_SUCCEED true
#define CACHED_FAILURE false

enum CacheNetType {
  NET_TYPE_WRITE = 1,
  NET_TYPE_READ = 2,
  NET_TYPE_DELETE = 3,
  NET_TYPE_RELEASECACHE = 4,
  NET_TYPE_SETPATHMODE = 5,
  NET_TYPE_ERROR = 6
};

struct CacheNetMessage {
  CacheNetMessage() :
      type(0), response_type(0), path_len(0),
      data_size(0), file_name_len(0), id(0) {
  }
  uint8 type;               //此次传输的类型，取值范围为CacheNetType内。
  uint8 response_type;      //回复的结果类型，取值为1代表成功或者0代表失败。
  uint16 path_len;          //path的数据长度
  uint32 data_size;         //data的数据长度
  uint32 file_name_len;     //file_name的数据长度
  uint32 id;                //自增id，唯一标识每一次数据传输。
  //char body[0];           //数据内容
};

namespace cache {

class CacheClient {
 public:
  typedef boost::shared_ptr<CacheClient> Ptr;

  sigslot::signal2<CacheClient::Ptr, int>
      SignalNetBreakEvent;

  // 创建一个新的CacheClient，每一个实例只需要一个
  // return 成功，CacheClient::Ptr；失败，NULL
  static CacheClient::Ptr CreateCacheClient();

  // 创建一个新的CacheClient，每一个实例只需要一个
  // return 成功，CacheClient::Ptr；失败，NULL
  static CacheClient::Ptr CreateCacheClient(vzes::SocketAddress addr);

  // 存储数据到指定的路径、文件，该接口为异步接口，线程安全。
  // file_name:文件路径。
  // 默认为相对路径， 绝对路径由cache service生成，并通过path[128]返回;
  // 可以通过SetPathMode(true)配置使用绝对路径，此时path[128]无返回值。
  // data:文件内容
  // data_size:文件内容长度，单位Byte
  // path:输出参数，文件存储“绝对”路径，如，/media/mmcblk0p0/exec/test.jpg，
  // SetPathMode(true)时无效。
  // return 成功，CACHED_SUCCEED；失败，CACHED_FAILURE
  virtual bool Write(const char *file_name, const char *data, int data_size,
                     char path[128]) = 0;

  // 存储数据到指定的路径、文件，该接口为异步接口，线程安全。
  // file_name:文件路径。
  // 默认为相对路径， 绝对路径由cache service生成，并通过path[128]返回;
  // 可以通过SetPathMode(true)配置使用绝对路径，此时path[128]无返回值。
  // data:文件内容
  // data_size:文件内容长度，单位Byte
  // path:输出参数，文件存储“绝对”路径，如，/media/mmcblk0p0/exec/test.jpg，
  // SetPathMode(true)时无效。
  // return 成功，CACHED_SUCCEED；失败，CACHED_FAILURE
  virtual bool Write(const char *file_name, vzes::MemBuffer::Ptr data,
                     char path[128]) = 0;

  // 读取文件，该接口为同步接口，线程安全。
  // path:文件绝对路径，如, /tmp/app/exec/test.jpg
  // return 成功，MemBuffer::Ptr；失败，NULL
  virtual MemBuffer::Ptr Read(const char *path) = 0;

  // 读取文件，该接口为异步接口，线程安全。
  // path:文件绝对路径，如, /tmp/app/exec/test.jpg
  // return 成功，CACHED_SUCCEED；失败，CACHED_FAILURE
  virtual bool Delete(const char *path) = 0;

  // 手动释放当前不在使用的缓存
  virtual void ReleaseCache() = 0;

  // 配置文件存储绝对路径生成模式
  // use_absolute_path:
  //   - true, 使用用户自定义的绝对路径
  //   - false, 用户自定义相对路径，绝对路径由cache service生成
  virtual void SetPathMode(bool use_abs_path) = 0;

  // 打印当前filecache缓存使用状况
  static void DumpCacheInfo();
};

}

#endif  // FILECHACHE_CACHE_CACHECLIENT_H_
