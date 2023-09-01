//
#include "filecache/server/cacheservice.h"
#include "eventservice/base/common.h"
#include "eventservice/base/timeutils.h"
#include "filecache/cache/cacheclient.h"
#include "filecache/server/cacheserver.h"
#include "json/json.h"
#include "log/log/log_client.h"

#ifndef WIN32
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>  // SYS_gettid
#include <sys/types.h>
#include <unistd.h>
#else
#include <fcntl.h>
#include <io.h>
#endif

namespace cache {

// file cache缓存大小
#ifdef WIN32
#define CACHE_POOL_SIZE (100)
#define CACHE_POOL_THRESHOLD (80)
#elif defined UBUNTU64
#define CACHE_POOL_SIZE (50)
#define CACHE_POOL_THRESHOLD (45)
#elif defined LITEOS
#define CACHE_POOL_SIZE (4)
#define CACHE_POOL_THRESHOLD (3)
#else  // linux
#ifdef CACHE_SIZE_LOW
#define CACHE_POOL_SIZE (4)
#define CACHE_POOL_THRESHOLD (3)
#else
#define CACHE_POOL_SIZE (30)
#define CACHE_POOL_THRESHOLD (25)
#endif
#endif

#define READ_FILE_BUFFER_SIZE (4096)
#define ASYNC_TIMEOUT_TIMES (10)  // 文件夹大小定时检查时间(S)
#define PART_FORMAT_LIMIT_FLASH (1)  // 多分区存储方案，Flash分区格式化阈值(MB)
#define PART_FORMAT_LIMIT_CARD (256)  // 多分区存储方案，SDCard分区格式化阈值(MB)
#define PART_SIZE_CHECK_COUNT (6)  // 多分区存储方案，分区容量检查阈值
#define FAULT_CARD_DIAG_LIMIT (6)  // 异常卡诊断次数，慢卡、错卡...
#define MAX_PATH_SIZE (128)

// 文件存储路径定义
#ifdef WIN32
#define PATH_NAME_FULL "c:\\vz_file_cache\\%s"
#elif defined UBUNTU64
#define PATH_NAME_FULL "/tmp/vz/media/%s"
#endif
#define PATH_NAME_CARD "card"  // CARD_PART_PATH_HEAD
#define PATH_NAME_BODY "/%s"

// 文件夹大小限制配置文件信息字段定义
#define JSON_FLC_LIMIT_CHECKS "limit_checks"
#define JSON_FLC_PATH "path"
#define JSON_FLC_MAX_SIZE "max_size"

CachedService::CachedService(vzes::EventService::Ptr es)
  : cache_size_(CACHE_POOL_SIZE), event_service_(es) {
  cachedstanza_pool_ = CachedStanzaPool::Instance();
  BOOST_ASSERT(cachedstanza_pool_ != NULL);
  // cachedstanza_pool_->SetDefaultCachedSize(CACHE_POOL_SIZE);

  disk_helper_ = vzes::DiskHelper::Instance(es);

  curr_part_idx_ = 0;
  Start();
}

CachedService::~CachedService() {}

bool CachedService::Start() {
  if (InitFileLimitCheck()) {
    StartFileLimitCheckTimer();
  }

  InitPartition();
  event_service_->PostDelayed(3000, this, CACHED_RENICE);
  return true;
}

void CachedService::OnMessage(vzes::Message *msg) {
  switch (msg->message_id) {
  case CACHED_ADD: {
    vzes::CritScope cr(&crit_);
    StanzaMessageData *stanza_msg =
      static_cast<StanzaMessageData *>(msg->pdata.get());
    if (stanza_msg) {
      (void)OnSaveFile(stanza_msg->stanza);
    }
    break;
  }
  case CACHED_FORMAT_PART: {
    vzes::CritScope cr(&crit_);
    FormatDeviceMessage *format_msg =
      static_cast<FormatDeviceMessage *>(msg->pdata.get());
    if (format_msg) {
      (void)FormatPartition(format_msg->device_idx,
                            format_msg->partition_idx_);
    }
    break;
  }
  case CACHED_CHECK: {
    CheckFileLimit();
    StartFileLimitCheckTimer();
    break;
  }
  case CACHED_RENICE: {
    OnRenice();
    break;
  }

  default:
    break;
  }
}

void CachedService::OnRenice() {
#if defined(WIN32) || defined(LITEOS)
  return;
#else
  char cmd[64 + 1];
  (void)snprintf(cmd, 64, "renice -n 10 -p %d", syscall(SYS_gettid));
  int ret = system(cmd);
  if (0 != ret) {
    DLOG_ERROR(MOD_EB, "renice vz_cacheSrv thread failed");
  } else {
    DLOG_INFO(MOD_EB, "renice vz_cacheSrv thread successed(nice:10)");
  }
#endif
}

int CachedService::OnSaveFile(CachedStanza::Ptr stanza) {
  if (NULL == stanza.get()) {
    DLOG_ERROR(MOD_EB, "invalid stanza ptr, save file failed");
    return -1;
  }

  uint32 bng_time = vzes::Time();
  static int io_error_times = 0;
  int res = AsyncSaveFile(stanza);
  if (0 > res) {
    // 文件I/O错误，卸载SD卡
    if (-2 == res) {
      io_error_times++;
      if (FAULT_CARD_DIAG_LIMIT == io_error_times) {
        DLOG_WARNING(MOD_EB,
                     "write I/O error %d times, set as abmormal part",
                     io_error_times);
        disk_helper_->InformError(sd_card_mounted_, curr_part_idx_, true);
        sd_card_mounted_ = false;
        io_error_times = 0;
      }
    } else if (-3 == res) {
      io_error_times = 0;
    }

    return -1;
  } else {
    io_error_times = 0;
    // 存文件成功，计算每32K写入速度，判断是否为慢速卡
    static uint32 slow_speed_times = 0;
    uint32 write_time = vzes::Time() - bng_time;
    const size_t PAG_SIZE = 32 * 1024;
    size_t file_size = stanza->size();
    uint32 pag_num =
      file_size / PAG_SIZE + (((file_size % PAG_SIZE) > 0) ? 1 : 0);
    uint32 per_page_time = (write_time / pag_num);
    if (per_page_time > 200) {
      slow_speed_times++;
      if (FAULT_CARD_DIAG_LIMIT == slow_speed_times) {
        DLOG_WARNING(MOD_EB,
                     "SD card write slow %d times, set as abmormal card",
                     slow_speed_times);
        disk_helper_->InformError(sd_card_mounted_, curr_part_idx_, false);
        sd_card_mounted_ = false;
        slow_speed_times = 0;
      }
    } else {
      slow_speed_times = 0;
    }

    DLOG_INFO(MOD_EB, "Writen file %s to flash done, size:%d, use %u(ms)",
              stanza->path().c_str(), file_size, write_time);
    return 0;
  }
}

// Stores files to disk
// returns:
// >0: successd, the length of stored
// -1: parameters error
// -2: file I/O error
// -3: Source data length error
int CachedService::AsyncSaveFile(CachedStanza::Ptr stanza) {
  if ((stanza->IsSaved()) || (NULL == stanza->data().get()) ||
      (NULL == stanza->path().c_str()) || (0 >= stanza->size())) {
    DLOG_ERROR(MOD_EB, "invalid stanza(path:%p, membuff:%p, saved:%d, size:%d)",
               stanza->path().c_str(), stanza->data().get(), stanza->IsSaved(),
               stanza->size());
    stanza->SaveConfimation();
    return -1;
  }
  // SD卡未插入时禁止打开卡上的文件，避免卸载卡失败
  //#ifdef MULTI_PART_STORAGE
  //  if (stanza->path().find(PATH_NAME_CARD) != std::string::npos) {
  //    if (!CheckSDCardStatus(NULL)) {
  //      DLOG_ERROR(MOD_EB, "SD Card not pluged, failed to save file %s",
  //                 stanza->path().c_str());
  //      stanza->SaveConfimation();
  //      return -1;
  //    }
  //  }
  //#endif

  if (curr_part_idx_ == (uint32)-1) {
    if (0 == disk_helper_->MountSDCard()) {
      InitPartition();
    }
  }

  CheckPartitionSize();

  char path[MAX_PATH_SIZE] = {0};
  GenFilePath(stanza->path().c_str(), path);
  if (path[0] == '\0') {
    DLOG_ERROR(MOD_EB, "get file path error, not invalid part to use.");
    return -1;
  }
  DLOG_INFO(MOD_EB, "save image path: %s\n", path);
  MakeDirRecursive(path);  // create filepath

#ifdef LITEOS
  int fp = open(path, O_CREAT | O_RDWR, 0666);
  if (-1 == fp)
#else
  FILE *fp = fopen(path, "wb");
  if (NULL == fp)
#endif
  {
    DLOG_ERROR(MOD_EB, "failed to open file %s, err: %d", path, errno);
    stanza->SaveConfimation();
    return -2;
  }

  int res = 0;
  std::size_t write_size = 0;
  vzes::BlocksPtr &block_list = stanza->data()->blocks();
  for (vzes::BlocksPtr::iterator iter = block_list.begin();
       iter != block_list.end(); iter++) {
    vzes::Block::Ptr block = *iter;
    const char *pdata = (const char *)block->buffer;
    std::size_t wrs = 0;
    int err = 0;
#ifdef LITEOS
    wrs = write(fp, pdata, block->buffer_size);
    err = errno;
    if (-1 == wrs)
#else
    wrs = fwrite(pdata, 1, block->buffer_size, fp);
    err = ferror(fp);
    if (err)
#endif
    {
      DLOG_ERROR(MOD_EB, "Write file error:%d", err);
      res = -2;
      break;
    }

    if (0 == wrs) {
      DLOG_ERROR(MOD_EB, "Write file error, size: 0");
      res = -2;
      break;
    }
    write_size += wrs;
  }
#ifdef LITEOS
  close(fp);
#else
  fclose(fp);
#endif

  if (0 == res) {
    if (write_size != stanza->size()) {
      DLOG_ERROR(MOD_EB, "write_size(%d) != cache_size(%d), source data error",
                 write_size, stanza->size());
      res = -3;
    } else {
      res = write_size;
    }
  }

  if (0 > res) {
    DLOG_ERROR(MOD_EB, "Save file %s failed, remove it", path);
    remove(path);
  }

  stanza->SaveConfimation();
  return res;
}

// 查找空闲空间最大的存储分区，如果SD卡在位，挂载卡并存储到卡上；
// 如果所有的分区空间都不充足，则选择并格式化第一个分区。
void CachedService::InitPartition() {
#if !defined(WIN32) && !defined(UBUNTU64)
  bool size_enough = false;
  uint32 size_limit = 0;
  PartitionInfo part_info;
  DLOG_INFO(MOD_EB, "Search free partition");
  bool sd_card_mounted = CheckSDCardStatus(NULL);
  if (sd_card_mounted) {
    size_limit = PART_FORMAT_LIMIT_CARD;
    curr_part_idx_ = disk_helper_->GetSDCardCurPart(size_limit, &part_info);
    DLOG_INFO(MOD_EB, "SD card Partition %d selected(total:%dMB, free:%dMB)",
              curr_part_idx_, part_info.total, part_info.free);
  }

  // SD卡损坏
  if (!sd_card_mounted && curr_part_idx_ == (uint32)-1) {
    sd_card_mounted_ = false;
    size_limit = PART_FORMAT_LIMIT_FLASH;
    curr_part_idx_ = disk_helper_->GetFlashCurPart(size_limit, &part_info);
    DLOG_INFO(MOD_EB, "SD card Partition %d selected(total:%dMB, free:%dMB)",
              curr_part_idx_, part_info.total, part_info.free);
  }

  // 分区不够空间, 格式化分区
  if (curr_part_idx_ != (uint32)-1 && part_info.free < size_limit) {
    DLOG_INFO(MOD_EB, "All partitions space insufficient, format the 1st part");
    FormatDeviceMessage *format_msg = new FormatDeviceMessage();
    format_msg->device_idx = sd_card_mounted;
    format_msg->partition_idx_ = curr_part_idx_;
    vzes::MessageData::Ptr msg_data(format_msg);
    event_service_->Post(this, CACHED_FORMAT_PART, msg_data);
  }
#endif
}

// 查询SD卡是否挂载：检查分区状态&数量、挂载
bool CachedService::CheckSDCardStatus(bool *old_stat) {
  if (old_stat) {
    *old_stat = sd_card_mounted_;
  }
#if !defined(WIN32) && !defined(UBUNTU64)
  sd_card_mounted_ = disk_helper_->IsSDCardMounted();
#endif
  return sd_card_mounted_;
}

// 格式化指定的SD卡分区，格式化成功后切换到该分区存储。
int CachedService::FormatPartition(uint32 device, uint32 part) {
  int ret = 0;
  uint32 cur_device = CheckSDCardStatus(NULL) ? 1 : 0;
  if (device == cur_device) {
    PartitionInfo part_info;
    uint32 nBng = vzes::Time();
    if (device) {
      ret = disk_helper_->FormatSDCardPartition(part);
      disk_helper_->GetSDCardPartSize(part, &part_info);
    } else {
      ret = disk_helper_->FormatFlashPartition(part);
      disk_helper_->GetFlashPartSize(part, &part_info);
    }
    if (0 == ret) {
      curr_part_idx_ = part;
    } else {
      // 格式化失败,使用另一个分区
      DLOG_INFO(MOD_EB, "format failed, need change another part %d", part);
      uint32 o_part = (part + 1) % SD_CARD_PART_NUM;
      if (o_part != part) {
        if (device) {
          ret = disk_helper_->FormatSDCardPartition(o_part);
          disk_helper_->GetSDCardPartSize(o_part, &part_info);
        } else {
          ret = disk_helper_->FormatFlashPartition(o_part);
          disk_helper_->GetFlashPartSize(o_part, &part_info);
        }
        if (0 == ret) {
          curr_part_idx_ = o_part;
        }
      }
    }

    uint32 nUseTime = vzes::Time() - nBng;
    DLOG_INFO(MOD_EB, "Format device %d partition %d done, ret:%d,"
              "use %u(ms), (total:%dMB, free:%dMB)",
              device, part, ret, nUseTime, part_info.total, part_info.free);
  } else {
    DLOG_WARNING(MOD_EB, "Storage device changed, terminate format");
  }
  return ret;
}

// 多分区方案：检查当前存储设备的存储分区剩余空间是否充足，
// 如果空间不足，则Post消息异步格式化其他的分区(当前分区index + 1)，
// 格式化成功后切换到该分区存储。
// 单分区方案：根据用户自定义的配置文件限制文件大小，这里不做检查。
void CachedService::CheckPartitionSize() {
  if (curr_part_idx_ == (uint32)-1) {
    return;
  }

#ifdef MULTI_PART_STORAGE
  int ret = 0;
  uint32 size_limit = 0;
  PartitionInfo part_info;
  if (sd_card_mounted_) {
    size_limit = PART_FORMAT_LIMIT_CARD;
    ret = disk_helper_->GetSDCardPartSize(curr_part_idx_, &part_info);
    DLOG_INFO(MOD_EB, "SD card part:%d,total:%dMB,free:%dMB,limit:%dMB)",
              curr_part_idx_, part_info.total, part_info.free, size_limit);
  } else {
    size_limit = PART_FORMAT_LIMIT_FLASH;
    ret = disk_helper_->GetFlashPartSize(curr_part_idx_, &part_info);
    DLOG_INFO(MOD_EB, "Flash part:%d,total:%dMB,free:%dMB,limit:%dMB)",
              curr_part_idx_, part_info.total, part_info.free, size_limit);
  }
  if (0 != ret) {
    // 获取分区size失败, 切换分区.
    curr_part_idx_ = (curr_part_idx_ + 1) % SD_CARD_PART_NUM;
  } else if (part_info.free <= size_limit) {
    DLOG_INFO(MOD_EB, "Current part %d space insufficient, save to part %d",
              curr_part_idx_, (curr_part_idx_ + 1) % SD_CARD_PART_NUM);
    FormatDeviceMessage *format_msg = new FormatDeviceMessage();
    if (format_msg) {
      // 设备index，flash:0, sd card：1
      format_msg->device_idx = sd_card_mounted_ ? 1 : 0;
      // 格式化另一个分区,做存储
      format_msg->partition_idx_ = (curr_part_idx_ + 1) % SD_CARD_PART_NUM;

      vzes::MessageData::Ptr msg_data(format_msg);
      event_service_->Post(this, CACHED_FORMAT_PART, msg_data);
    }
  }
#endif
}

// 创建各级文件夹
void CachedService::MakeDirRecursive(const char *pPath) {
  if ((NULL == pPath) || (0 == strlen(pPath))) {
    return;
  }

  char buf[512];
  strncpy(buf, pPath, 512);
  int len = strlen(pPath);
#ifdef WIN32
  // 去除文件名
  char *last = strrchr(buf, '\\');
  if (last) {
    last[0] = '\0';
  }
  // 创建中间路径
  for (int i = 3; i < len; i++) {
    if (buf[i] == '\\') {
      buf[i] = '\0';
      ::CreateDirectory(buf, NULL);
      buf[i] = '\\';
    }
  }
  // 创建最后一级文件夹
  ::CreateDirectory(buf, NULL);
#else
  char *last = strrchr(buf, '/');
  if (last) {
    last[0] = '\0';
  }
  for (int i = 1; i < len; i++) {
    if (buf[i] == '/') {
      buf[i] = '\0';
      if (access(buf, 0) != 0) {
        mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
      }
      buf[i] = '/';
    }
  }
  if (len > 0 && access(buf, 0) != 0) {
    mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
  }
#endif
}

// 删除文件夹中文件，包括子文件夹
// folder_path文件夹路径
bool CachedService::RemoveFolderFiles(std::string folder_path) {
#ifdef WIN32
  char temp_path[MAX_PATH_SIZE] = {0};
  WIN32_FIND_DATA file_data;
  std::string file_name;
  DWORD64 file_size = 0;

  file_name = folder_path;
  if (0 == file_name.size()) {
    return false;
  }

  if (3 == file_name.size() && '\\' == file_name[2]) {
    file_name += "*";
  } else {
    file_name += "\\*";
  }
  HANDLE hLisFile = ::FindFirstFile(file_name.data(), &file_data);
  if (INVALID_HANDLE_VALUE == hLisFile) {
    return false;
  }

  do {
    if (!strcmp(file_data.cFileName, ".") ||
        !strcmp(file_data.cFileName, "..")) {
      continue;
    }
    if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      snprintf(temp_path, MAX_PATH_SIZE, "%s%s%s", folder_path.c_str(), "\\",
               file_data.cFileName);
      // 递归查找子文件夹
      file_size += RemoveFolderFiles(temp_path);
    } else {
      ::DeleteFile(
        std::string(folder_path + "\\" + file_data.cFileName).c_str());
    }
  } while (::FindNextFile(hLisFile, &file_data));

  ::FindClose(hLisFile);
  return true;
#else
  DIR *dir;
  struct dirent *ptr;
  char path[MAX_PATH_SIZE] = {0};
  const char *dirname = folder_path.c_str();

  dir = opendir(dirname);
  if (dir == NULL) {
    DLOG_WARNING(MOD_EB, "open dir %s failed.\n", path);
    return false;
  }

  while ((ptr = readdir(dir)) != NULL) {
    snprintf(path, (size_t)MAX_PATH_SIZE, "%s/%s", dirname, ptr->d_name);
    if (strcmp(ptr->d_name, ".") == 0) {
      continue;
    }
    if (strcmp(ptr->d_name, "..") == 0) {
      continue;
    }
#ifdef LITEOS
    // Liteos系统，readdir读取文件夹属性错误，
    // 即ptr->d_type！=DT_DIR
    struct stat st;
    int rt = stat(path, &st);
    if (rt >= 0 && S_ISDIR(st.st_mode))
#else
    if (ptr->d_type == DT_DIR)
#endif
    {
      (void)RemoveFolderFiles(path);
      memset(path, 0, sizeof(path));
    } else {
      // total_size += buf.st_size;
      remove(path);
    }
  }

  closedir(dir);
  return true;
#endif
}

// 获取文件夹中文件大小，包括子文件夹
// folder_path文件夹路径
uint64 CachedService::GetFolderSize(std::string folder_path) {
#ifdef WIN32
  char temp_path[MAX_PATH_SIZE] = {0};
  WIN32_FIND_DATA file_data;
  std::string file_name;
  DWORD64 file_size = 0;

  file_name = folder_path;
  if (0 == file_name.size()) {
    return 0;
  }

  if (3 == file_name.size() && '\\' == file_name[2]) {
    file_name += "*";
  } else {
    file_name += "\\*";
  }
  HANDLE hLisFile = ::FindFirstFile(file_name.data(), &file_data);
  if (INVALID_HANDLE_VALUE == hLisFile) {
    return 0;
  }

  do {
    if (!strcmp(file_data.cFileName, ".") ||
        !strcmp(file_data.cFileName, "..")) {
      continue;
    }
    if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      snprintf(temp_path, MAX_PATH_SIZE, "%s%s%s", folder_path.c_str(), "\\",
               file_data.cFileName);
      // 递归查找子文件夹
      file_size += GetFolderSize(temp_path);
    } else {
      // 高低字位的组合
      DWORD64 dwFileTempHigh = file_data.nFileSizeHigh;
      DWORD64 dwFileTempLow = file_data.nFileSizeLow;
      dwFileTempHigh = (dwFileTempHigh << 32);
      dwFileTempHigh = (dwFileTempHigh | dwFileTempLow);
      file_size += dwFileTempHigh;
    }
  } while (::FindNextFile(hLisFile, &file_data));

  ::FindClose(hLisFile);
  return (uint64)file_size;
#else
  DIR *dir;
  struct dirent *ptr;
  uint64 total_size = 0;
  char path[MAX_PATH_SIZE] = {0};
  const char *dirname = folder_path.c_str();

  dir = opendir(dirname);
  if (dir == NULL) {
    DLOG_WARNING(MOD_EB, "open dir %s failed.\n", path);
    return 0;
  }

  while ((ptr = readdir(dir)) != NULL) {
    snprintf(path, (size_t)MAX_PATH_SIZE, "%s/%s", dirname, ptr->d_name);
    struct stat buf;
    if (lstat(path, &buf) < 0) {
      DLOG_WARNING(MOD_EB, "lstat %s error.\n", path);
    }
    if (strcmp(ptr->d_name, ".") == 0) {
      total_size += buf.st_size;
      continue;
    }
    if (strcmp(ptr->d_name, "..") == 0) {
      continue;
    }
#ifdef LITEOS
    // Liteos系统，readdir读取文件夹属性错误，
    // 即ptr->d_type！=DT_DIR
    struct stat st;
    int rt = stat(path, &st);
    if (rt >= 0 && S_ISDIR(st.st_mode))
#else
    if (ptr->d_type == DT_DIR)
#endif
    {
      total_size += GetFolderSize(path);
      memset(path, 0, sizeof(path));
    } else {
      total_size += buf.st_size;
    }
  }

  closedir(dir);
  return total_size;
#endif
}

// 检查各个被监控文件夹大小，超过limit大小
// 则删除文件夹中的所有文件
void CachedService::CheckFileLimit() {
  for (uint32 i = 0; i < flc_stanzas_.size(); i++) {
    FlcStanza &stanza = flc_stanzas_[i];
    uint64 file_size = 0;
    vzes::TimeVal tv_start, tv_stop;
    vzes::TimeOfDay(&tv_start, NULL);
    file_size = GetFolderSize(stanza.path);
    if (stanza.max_size < file_size) {
      RemoveFolderFiles(stanza.path);
      DLOG_INFO(MOD_EB, "File <%s> size %ld, exceed limit size %ld",
                stanza.path.c_str(), file_size, stanza.max_size);
    }
    vzes::TimeOfDay(&tv_stop, NULL);
    DLOG_INFO(MOD_EB, "Checking file <%s> size takes %dusec",
              stanza.path.c_str(),
              (tv_stop.sec - tv_start.sec) * 1000 * 1000 +
              (tv_stop.usec - tv_start.usec));
  }
}

// 解析Filecache文件夹大小配置文件
bool CachedService::InitFileLimitCheck() {
  // Open this file
  FILE *fp = fopen(FILE_LIMIT_CONFIG, "rb");
  if (fp == NULL) {
    DLOG_WARNING(MOD_EB, "failed to open config file %s, err: %d",
                 FILE_LIMIT_CONFIG, errno);
    return false;
  }

  do {
    // Read the file data
    char temp[128];
    std::string data;
    while (true) {
      std::size_t read_size = fread(temp, sizeof(char), 128, fp);
      if (read_size) {
        data.append(temp, read_size);
      } else {
        break;
      }
    }
    // Check if the file data is empty
    if (data.empty()) {
      DLOG_ERROR(MOD_EB, "config file %s is empty", FILE_LIMIT_CONFIG);
      break;
    }
    // Parse the file data to flc_stanzas_
    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(data, value)) {
      DLOG_ERROR(MOD_EB, "parse file data error %s",
                 reader.getFormattedErrorMessages().c_str());
      break;
    }

    Json::Value limit_checks = value[JSON_FLC_LIMIT_CHECKS];
    for (Json::ArrayIndex i = 0; i < limit_checks.size(); i++) {
      if (limit_checks[i][JSON_FLC_PATH].isNull() ||
          limit_checks[i][JSON_FLC_MAX_SIZE].isNull()) {
        DLOG_ERROR(MOD_EB, "Invalid intem");
        continue;
      }
      FlcStanza stanza;
      stanza.max_size = limit_checks[i][JSON_FLC_MAX_SIZE].asUInt() * 1024;
      stanza.path = limit_checks[i][JSON_FLC_PATH].asString();
      DLOG_INFO(MOD_EB, "file %s size limit: %d", stanza.path.c_str(),
                stanza.max_size);
      flc_stanzas_.push_back(stanza);
    }
  } while (0);

  fclose(fp);
  if (flc_stanzas_.size()) {
    return true;
  }
  return false;
}

// 定时检查Filecache文件夹大小
void CachedService::StartFileLimitCheckTimer() {
  event_service_->PostDelayed(ASYNC_TIMEOUT_TIMES * 1000, this, CACHED_CHECK);
}

// 检查Stanza缓存是否在使用，可被释放的条件：
// - 文件已经被写入flash
// - 当前没有用户使用该文件，即MemBuffer::Ptr的引用计数等于1
// 注意：调用该函数前请勿引入新的stanza引用计数
bool CachedService::IsStanzaFree(CachedStanza::Ptr stanza) {
  MemBuffer::Ptr mbuff = stanza->data();
  if (stanza->IsSaved() && ((stanza.use_count() - 1) == 1) &&
      ((mbuff.use_count() - 1) == 1)) {
    return true;
  }
  return false;
}

// 查询SD卡状态，生成存储路径
void CachedService::GenFilePath(const char *name, char path[MAX_PATH_SIZE]) {
  if (curr_part_idx_ == (uint32)-1) {
    return;
  }

#if !defined(WIN32) && !defined(UBUNTU64)
  bool old_stat = false;
  bool card_pluged = CheckSDCardStatus(&old_stat);
  if (card_pluged) {
    // 有新卡插入并挂载，重新选择空闲空间最大的卡分区，切换到该分区存储
    if (card_pluged != old_stat) {
      PartitionInfo part_info;
      uint32 size_limit = PART_FORMAT_LIMIT_CARD;
      int ret = disk_helper_->GetSDCardCurPart(size_limit, &part_info);
      if (ret >= 0) {
        curr_part_idx_ = ret;
        DLOG_INFO(MOD_EB,
                  "SD card pluged in, save to part %d(total:%d, free:%d)", ret,
                  part_info.total, part_info.free);
      }
    }
    int size =
      snprintf(path, MAX_PATH_SIZE, CARD_PART_PATH_HEAD, curr_part_idx_ + 1);
    size = snprintf(path + size, MAX_PATH_SIZE - size, PATH_NAME_BODY, name);
  } else {
    // 卡被拔出，重新选择空闲空间最大的Flash分区，切换到该分区存储
    if (card_pluged != old_stat) {
      PartitionInfo part_info;
      uint32 size_limit = PART_FORMAT_LIMIT_FLASH;
      int ret = disk_helper_->GetFlashCurPart(size_limit, &part_info);
      if (ret >= 0) {
        curr_part_idx_ = ret;
        DLOG_INFO(MOD_EB,
                  "SD card pluged out, save to flash part %d"
                  "(total:%d, free:%d)",
                  ret, part_info.total, part_info.free);
      }
    }
    int size =
      snprintf(path, MAX_PATH_SIZE, FLASH_PART_PATH_HEAD, curr_part_idx_ + 1);
    size = snprintf(path + size, MAX_PATH_SIZE - size, PATH_NAME_BODY, name);
  }
#else
  snprintf(path, MAX_PATH_SIZE, PATH_NAME_FULL, name);
#endif
}

bool CachedService::AddFile(CachedStanza::Ptr stanza, bool is_cached) {
  ReplaceCachedFile(stanza);
  if (is_cached) {
    // the number of caches reached the threshold, the current stanza is
    // not save to flash.
    int size = cached_stanzas_.size();
    if (size > CACHE_POOL_THRESHOLD) {
      DLOG_WARNING(MOD_EB,
                   "cached stanzas num %d > %d, not save file %s to flash",
                   size, CACHE_POOL_THRESHOLD, stanza->path().c_str());
      stanza->SaveConfimation();
      return true;
    }

    StanzaMessageData *stanza_msg = new StanzaMessageData();
    stanza_msg->stanza = stanza;
    vzes::MessageData::Ptr msg_data(stanza_msg);
    event_service_->Post(this, CACHED_ADD, msg_data);
  } else {
    // Set the "saved" flag, make sure this stanza can be recycled
    // in all scenarios.
    stanza->SaveConfimation();
  }

  return true;
}

bool CachedService::SaveFile(const std::string file_name,
                             MemBuffer::Ptr data) {
  vzes::CritScope cr(&crit_);
  RemoveOutOfDataStanza(false);
  CachedStanza::Ptr stanza = CachedStanzaPool::Instance()->TakeStanza();
  if (stanza) {
    stanza->SetPath(file_name.c_str());
    stanza->SetData(data);
    // CheckPartitionSize();
    return AddFile(stanza, true);
  } else {
    DLOG_WARNING(MOD_EB, "save file %s failed, no free stanzas",
                 file_name.c_str());
    return false;
  }
}

bool CachedService::SaveFile(const std::string file_name, MemBuffer::Ptr data,
                             std::string &abs_path_name) {
  // char path[MAX_PATH_SIZE] = {0};
  // GenFilePath(file_name.c_str(), path);
  // abs_path_name = path;
  abs_path_name = file_name;
  return SaveFile(abs_path_name, data);
}

// 将文件存入缓存，如果在缓存中已经存在该文件，
// 则替换旧值
void CachedService::ReplaceCachedFile(CachedStanza::Ptr stanza) {
  // 去掉重复的元素
  for (std::size_t i = 0; i < cached_stanzas_.size(); i++) {
    if (cached_stanzas_[i]->path() == stanza->path()) {
      cached_stanzas_.erase(cached_stanzas_.begin() + i);
    }
  }
  cached_stanzas_.push_back(stanza);

  static unsigned int nCount = 0;
  DLOG_DEBUG(MOD_EB,
             "total times:%d, stanza size:%d, mbuff blocks:%d,"
             "mbuff:%d, cachedata:%d, CachedStanza:%d",
             ++nCount, cached_stanzas_.size(), vzes::g_block_count,
             vzes::g_membuffer_count, g_cache_data_count,
             CachedStanza::stanza_count);
}

bool CachedService::RemoveFile(const std::string path) {
  vzes::CritScope cr(&crit_);
  for (std::deque<CachedStanza::Ptr>::iterator iter = cached_stanzas_.begin();
       iter != cached_stanzas_.end(); ++iter) {
    if ((*iter)->path() == path) {
      if (IsStanzaFree(*iter)) {
        cached_stanzas_.erase(iter);
      }
      break;
    }
  }
  OnAsyncRemoveFile(path);
  return true;
}

void CachedService::OnAsyncRemoveFile(std::string path) {
  remove(path.c_str());
}

MemBuffer::Ptr CachedService::GetFile(const std::string path) {
  vzes::CritScope cr(&crit_);
  for (std::deque<CachedStanza::Ptr>::iterator iter = cached_stanzas_.begin();
       iter != cached_stanzas_.end(); ++iter) {
    if ((*iter)->path() == path) {
      uint32 size = (*iter)->data()->size();
      if (FILE_MIN_SIZE > size) {
        DLOG_ERROR(MOD_EB, "Get file failed, invalid file size: %d", size);
        return MemBuffer::Ptr();
      }
      vzes::BlocksPtr &block_list = (*iter)->data()->blocks();
      vzes::Block::Ptr block = block_list.back();
      if (((int)(unsigned char)block->buffer[block->buffer_size - 1] != 0xD9)) {
        DLOG_WARNING(MOD_EB, "read file %s from cache,size:%d,last byte:%X",
                     path.c_str(), (*iter)->size(),
                     (int)(unsigned char)block->buffer[block->buffer_size - 1]);
      }
      return (*iter)->data();
    }
  }

  RemoveOutOfDataStanza(false);
  CachedStanza::Ptr stanza = cachedstanza_pool_->TakeStanza();
  if (!stanza) {
    DLOG_WARNING(MOD_EB, "Get file %s failed, no free stanzas", path.c_str());
    return MemBuffer::Ptr();
  }

  char abs_path[MAX_PATH_SIZE] = {0};
  if (disk_helper_) {
    vzes::DiskHelper::GetABSFilePath(path.c_str(), abs_path);
  }

  MemBuffer::Ptr data_buff = stanza->data();
  stanza->SetPath(abs_path);
  if (ReadFile(stanza->path(), data_buff)) {
    // 存入缓存。注意: stanza的生命周期必须和MemBuffer一致，
    // 即Stanza和MemBuffer一一对应，否则会导致实际分配的MemBuffer
    // 数量大于Stanza的数量(即缓存的数量).
    AddFile(stanza, false);
    return data_buff;
  }
  DLOG_WARNING(MOD_EB, "Get file failed: %s", path.c_str());
  return MemBuffer::Ptr();
}

void CachedService::ReleaseCache() {
  vzes::CritScope cr(&crit_);
  RemoveOutOfDataStanza(true);
}

void CachedService::DumpCacheInfo() {
  vzes::CritScope cr(&crit_);
  int32 size = 0;
  int32 index = 0;
  printf("File Cache stanza usage:\n");
  printf(
    "> index | mbuff block num | saved | stanza count | mbuff count | "
    "file\n");
  std::deque<CachedStanza::Ptr>::iterator iter;
  for (iter = cached_stanzas_.begin(); iter != cached_stanzas_.end(); ++iter) {
    CachedStanza::Ptr stanza = (*iter);
    MemBuffer::Ptr mbuff = stanza->data();
    vzes::BlocksPtr &blocks = mbuff->blocks();
    uint32 block_num = blocks.size();
    size += block_num;
    index++;
    printf("> %-2d | %-4d | %-s | %-d | %-d | %-s\n", index, block_num,
           (stanza->IsSaved() ? "Y" : "N"), stanza.use_count() - 2,
           mbuff.use_count() - 2, stanza->path().c_str());
  }
  printf("> total %d stanzas, %d membuffer blocks\n", index, size);
}

bool CachedService::ReadFile(const std::string path,
                             MemBuffer::Ptr data_buffer) {
  if (NULL == data_buffer.get()) {
    DLOG_ERROR(MOD_EB, "Invalid data buffer");
    return false;
  }

  static char read_buffer[READ_FILE_BUFFER_SIZE];
  uint32 total_size = 0;
  int read_size = 0;
//#ifdef MULTI_PART_STORAGE
//  // SD卡未插入时禁止打开卡上的文件，避免卸载卡失败
//  if (path.find(PATH_NAME_CARD) != std::string::npos) {
//    if (!CheckSDCardStatus(NULL)) {
//      DLOG_ERROR(MOD_EB, "SD Card not pluged, can not read file %s",
//                 path.c_str());
//      return false;
//    }
//  }
//#endif
#ifdef LITEOS
  // LITEOS系统fopen、fread接口易出错，使用open、read接口
  int fd = open(path.c_str(), O_RDONLY);
  if (0 >= fd) {
    DLOG_ERROR(MOD_EB, "Failed to open file %s, err: %d", path.c_str(), errno);
    return false;
  }

  do {
    read_size = read(fd, (void *)read_buffer, READ_FILE_BUFFER_SIZE);
    if (read_size > 0) {
      data_buffer->WriteBytes(read_buffer, read_size);
      total_size += read_size;
    } else {
      break;
    }
  } while (read_size > 0);
  close(fd);
  fd = 0;
#else
  FILE *fd = fopen(path.c_str(), "rb");
  if (NULL == fd) {
    DLOG_ERROR(MOD_EB, "Failed to open file %s, err: %d", path.c_str(), errno);
    return false;
  }
  while (!feof(fd)) {
    read_size = fread((void *)read_buffer, 1, READ_FILE_BUFFER_SIZE, fd);
    if (read_size > 0) {
      data_buffer->WriteBytes(read_buffer, read_size);
      total_size += read_size;
    } else {
      break;
    }
  }
  fclose(fd);
  fd = NULL;
#endif

  if (FILE_MIN_SIZE > total_size) {
    DLOG_ERROR(MOD_EB, "Invalid file, size: %dByte", total_size);
    return false;
  }

  vzes::BlocksPtr &block_list = data_buffer->blocks();
  vzes::Block::Ptr block = block_list.back();
  if (((int)(unsigned char)block->buffer[block->buffer_size - 1] != 0xD9)) {
    DLOG_WARNING(MOD_EB, "read file %s from flash,size:%d, last byte:%X",
                 path.c_str(), total_size,
                 (int)(unsigned char)block->buffer[block->buffer_size - 1]);
  }
  return true;
}

// remove_all：true，清空全部缓存。
// false, 清楚缓存直到缓存数量小于等于缓存总量的1/2。
void CachedService::RemoveOutOfDataStanza(bool remove_all) {
  if (!remove_all && (cached_stanzas_.size() <= (CACHE_POOL_SIZE / 2))) {
    return;
  }

  std::deque<CachedStanza::Ptr>::iterator iter;
  for (iter = cached_stanzas_.begin(); iter != cached_stanzas_.end();) {
    if (IsStanzaFree(*iter)) {
      DLOG_DEBUG(MOD_EB, "Recycle unused stanza: %s", (*iter)->path().c_str());
      iter = cached_stanzas_.erase(iter);
      if (!remove_all && (cached_stanzas_.size() <= (CACHE_POOL_SIZE / 2))) {
        break;
      }
    } else {
      ++iter;
    }
  }

  DLOG_DEBUG(MOD_EB, "stanza size:%d", cached_stanzas_.size());
}
}
