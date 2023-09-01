//
#include <stdio.h>
#ifndef WIN32
#include <sys/stat.h>
#include <sys/statfs.h>
#else
#include <io.h>
#endif
#include "astl/mem_dump.h"
#include "filecache/base/diskhelper.h"

#define TIMER_CHECK_SDCARD (10 * 1000)
#define BLOCK_DEV_NUM_SDCARD (0)  // Block Device SD卡index

namespace vzes {

DiskHelper *DiskHelper::instance_ = NULL;

DiskHelper *DiskHelper::Instance(EventService::Ptr es) {
  if (!instance_) instance_ = new DiskHelper(es);
  return instance_;
}

DiskHelper::DiskHelper(EventService::Ptr es) : event_service_(es) {
  DLOG_INFO(MOD_EB, "Construct DiskHelper");
#if !defined(WIN32) && !defined(UBUNTU64)
  async_mounting_ = false;
  memset(&part_info_, 0, sizeof(part_info_));
  OnMessage(NULL);  // mount sd, loop check
#endif
}

DiskHelper::~DiskHelper() {
  DLOG_INFO(MOD_EB, "Destruct DiskHelper");
}

void DiskHelper::OnMessage(vzes::Message *message) {
  //MountSDCard();

  //AsyncCheckSDCard();
}

unsigned int DiskHelper::SD_PART_NUM() {
  return SD_CARD_PART_NUM;
}

// #if !defined(WIN32) && !defined(UBUNTU64)
void DiskHelper::AsyncCheckSDCard(void) {
  // vzes::CritScope cs(&crit_);
  DLOG_DEBUG(MOD_EB, "Async Check SD card, stat:%d", async_mounting_);
  event_service_->PostDelayed(TIMER_CHECK_SDCARD, this);
}

int DiskHelper::UnMountSDCard(uint32 part_id) {
#ifndef _WIN32
  DLOG_INFO(MOD_EB, "UnMount SD card request");
  if (part_id >= SD_CARD_PART_NUM) {
    DLOG_ERROR(MOD_EB, "UnMount SD partition %d failed", part_id);
    return -1;
  }

  vzes::CritScope cs(&crit_);
  int ret =
    VZ_DeviceSDK_BlockDevice_UnMount(part_info_.card_part[part_id].dev_path);
  if (0 > ret) {
    DLOG_ERROR(MOD_EB, "UnMount SD card partition %d failed", part_id);
    return -1;
  }
#endif
  return 0;
}

int DiskHelper::MountSDCard(void) {
#ifndef _WIN32
  DLOG_INFO(MOD_EB, "Mount SD card request");
  vzes::CritScope cs(&crit_);
  VzBlockDeviceInfo *dev_info = NULL;
  dev_info = VZ_DeviceSDK_BlockDevice_GetInfo();
  if (!dev_info) {
    DLOG_ERROR(MOD_EB, "get device info failed");
    return -1;
  }

  int dev_idx = 0;
  bool find_sd_card = false;
  // 此处存在风险: 只认一个SD卡,多余的不处理.
  for (dev_idx = 0; dev_idx < dev_info->BlockDevNum; dev_idx++) {
    if (BLOCK_DEVICE_SD == dev_info->BlockDev[dev_idx].BlockDevFlag) {
      find_sd_card = true;
      break;
    }
  }
  if (0 >= dev_info->BlockDevNum || false == find_sd_card) {
    // DLOG_WARNING(MOD_EB, "detect %d devices, mount SD card failed",
    //              dev_info->BlockDevNum);

    // 没插入SD卡, 删除挂载信息
    for (int i = 0; i < SD_CARD_PART_NUM; i++) {
      memset(&part_info_.card_part[i], 0, sizeof(TAG_PART_INFO));
    }
    return -1;
  }

  VzBlockDevice &sd_part_info = dev_info->BlockDev[dev_idx];
  // 分区数不一致
  if (SD_CARD_PART_NUM != sd_part_info.PartNum) {
    DLOG_INFO(MOD_EB, "SD card partitions num:%d != %d, repart it firstly",
              sd_part_info.PartNum, SD_CARD_PART_NUM);
    return -1;
  }

  // 挂载分区
  bool new_mounted = false;
  for (int part_idx = 0; part_idx < sd_part_info.PartNum; part_idx++) {
    DLOG_DEBUG(MOD_EB, "partition %d, flag %d", part_idx,
               sd_part_info.PartInfo[part_idx].PartFlag);
    // 初始化part_info
    if (0 != strcmp(part_info_.card_part[part_idx].dev_path,
                    sd_part_info.PartInfo[part_idx].PartDevPath)) {
      strncpy(part_info_.card_part[part_idx].dev_path,
              sd_part_info.PartInfo[part_idx].PartDevPath,
              VZ_BLOCK_DEVICE_PATH_SIZE);
    }
    if (PART_MOUNTED == sd_part_info.PartInfo[part_idx].PartFlag) {
      part_info_.card_part[part_idx].flag = PART_MOUNTED;
      if (0 != strcmp(part_info_.card_part[part_idx].mount_path,
                      sd_part_info.PartInfo[part_idx].PartMountPath)) {
        strncpy(part_info_.card_part[part_idx].mount_path,
                sd_part_info.PartInfo[part_idx].PartMountPath,
                VZ_BLOCK_DEVICE_PATH_SIZE);
      }
    } else if (PART_UNMOUNT != sd_part_info.PartInfo[part_idx].PartFlag) {
      part_info_.card_part[part_idx].flag = PART_ERROR;
    }

    // 挂载
    if (PART_UNMOUNT == sd_part_info.PartInfo[part_idx].PartFlag) {
      char part_name[64] = {0};
      snprintf(part_name, 64, (char *)CARD_PART_PATH_HEAD, (part_idx + 1));
      int ret = VZ_DeviceSDK_BlockDevice_Mount(
                  sd_part_info.PartInfo[part_idx].PartDevPath, part_name);
      if (0 > ret) {
        DLOG_ERROR(MOD_EB, "Mount partition %d failed, ret:%d", part_idx, ret);
        part_info_.card_part[part_idx].flag = PART_ERROR;
        return -1;
      }
      DLOG_INFO(MOD_EB, "Partition %d: %s, mount to path %s successed",
                part_idx, sd_part_info.PartInfo[part_idx].PartDevPath,
                part_name);

      part_info_.card_part[part_idx].flag = PART_MOUNTED;
      if (0 != strcmp(part_info_.card_part[part_idx].mount_path, part_name)) {
        strncpy(part_info_.card_part[part_idx].mount_path, part_name,
                VZ_BLOCK_DEVICE_PATH_SIZE);
      }
      new_mounted = true;
    }
  }

  if (new_mounted) {
    DLOG_INFO(MOD_EB, "New SD card mount successed");
  } else {
    DLOG_INFO(MOD_EB, "SD card already mounted");
  }
#endif
  return 0;
}

bool DiskHelper::IsSDCardMounted(void) {
#ifndef _WIN32
  unsigned int disk_total = 0;
  for (int part_idx = 0; part_idx < SD_CARD_PART_NUM; part_idx++) {
    char part_name[64] = {0};
    snprintf(part_name, 64, CARD_PART_PATH_HEAD, (part_idx + 1));

    PartitionInfo part_info;
    GetPartInfo(part_name, &part_info);

    // 挂载成功才能视为OK
    if (PART_MOUNTED == part_info_.card_part[part_idx].flag) {
      disk_total += part_info.total;

      DLOG_INFO(MOD_EB, "part %s, total %u used %u free %u.", part_name,
                part_info.total, part_info.used, part_info.free);
    }
  }
  if (disk_total > 1024) {  // more than 1024MB, it's SD.
    return true;
  }
#endif
  return false;
}

int DiskHelper::FormatSDCardPartition(uint32 part_id) {
#ifndef _WIN32
  DLOG_INFO(MOD_EB, "Format sd card partition %d request", part_id);
  if (part_id >= SD_CARD_PART_NUM) {
    DLOG_ERROR(MOD_EB, "Format sd card partition %d failed", part_id);
    return -1;
  }

  int ret = -1;
  {
    // 避免与MountSDCard的锁有重入
    vzes::CritScope cs(&crit_);
    ret = VZ_DeviceSDK_BlockDevice_Format(part_info_.card_part[part_id].dev_path);
    if (0 > ret) {
      DLOG_ERROR(MOD_EB, "Format SD card partition %d failed", part_id);
      part_info_.card_part[part_id].flag = PART_ERROR;
      return -1;
    }
    DLOG_INFO(MOD_EB, "Format SD card partition %s successed",
              part_info_.card_part[part_id].dev_path);
  }

  // 格式化之后立即mount上磁盘
  if (0 == ret) {
    MountSDCard();
  }

  if (PART_MOUNTED == part_info_.card_part[part_id].flag) {
    return 0;
  }
#endif
  return -1;
}

int DiskHelper::FormatSDCardPartition(std::string dev_path) {
#ifndef _WIN32
  int ret = -1;
  {
    // 避免与MountSDCard的锁有重入
    vzes::CritScope cs(&crit_);
    ret = VZ_DeviceSDK_BlockDevice_Format((char*)dev_path.c_str());
    if (0 > ret) {
      DLOG_ERROR(MOD_EB, "Format SD card partition %s failed", dev_path.c_str());
      return -1;
    }
    DLOG_INFO(MOD_EB, "Format SD card partition %s successed", dev_path.c_str());
  }

  // 格式化之后立即mount上磁盘
  if (0 == ret) {
    MountSDCard();
  }
#endif
  return 0;
}

int DiskHelper::SplitPartSDCard(void) {
#ifndef _WIN32
  bool is_need_mount = false;
  {
    // 避免与MountSDCard的锁有重入
    vzes::CritScope cs(&crit_);
    VzBlockDeviceInfo *dev_info = NULL;
    dev_info = VZ_DeviceSDK_BlockDevice_GetInfo();
    if (!dev_info) {
      DLOG_ERROR(MOD_EB, "get device info failed");
      return -1;
    }

    // 此处存在风险: 只认一个SD卡,多余的不处理.
    for (int dev_idx = 0; dev_idx < dev_info->BlockDevNum; dev_idx++) {
      if (BLOCK_DEVICE_SD == dev_info->BlockDev[dev_idx].BlockDevFlag) {
        VzBlockDevice &sd_part_info = dev_info->BlockDev[dev_idx];
        if (SD_CARD_PART_NUM != sd_part_info.PartNum) {
          is_need_mount = true;
          VZ_DeviceSDK_BlockDevice_Partition(
            dev_info->BlockDev[dev_idx].BlockDevPath, SD_CARD_PART_NUM);
        }
        break;
      }
    }
  }

  if (is_need_mount) {
    MountSDCard();
  }
#endif
  return 0;
}

int DiskHelper::FormatFlashPartition(uint32 part_id) {

#ifndef _WIN32
  DLOG_INFO(MOD_EB, "Format flash partition %d request", part_id);
  if (part_id >= SD_CARD_PART_NUM) {
    DLOG_ERROR(MOD_EB, "Format flash partition %d failed", part_id);
    return -1;
  }
  int ret = VZ_DeviceSDK_Nand_Format_Media(part_id);
  if (0 > ret) {
    part_info_.nand_part[part_id].flag = PART_ERROR;
    DLOG_ERROR(MOD_EB, "Format flash partition %d failed", part_id);
  } else {
    part_info_.nand_part[part_id].flag = PART_MOUNTED;
    DLOG_INFO(MOD_EB, "Format flash partition %d successed", part_id);
  }
#endif
  return 0;
}

int DiskHelper::FormatFlash(void) {
#ifndef _WIN32
  DLOG_INFO(MOD_EB, "Format flash request");
  for (int i = 0; i < SD_CARD_PART_NUM; i++) {
    int ret = VZ_DeviceSDK_Nand_Format_Media(i);
    if (0 > ret) {
      DLOG_ERROR(MOD_EB, "Format flash partition %d failed", i);
    } else {
      DLOG_INFO(MOD_EB, "Format flash partition %d successed", i);
    }
  }
#endif
  return 0;
}

int DiskHelper::GetFlashPartSize(uint32 part_id, PartitionInfo *info) {
#ifndef _WIN32
  if (NULL == info) {
    DLOG_ERROR(MOD_EB, "Get flash partition size faile, invalid para");
    return -1;
  }
  if (part_id >= SD_CARD_PART_NUM) {
    DLOG_ERROR(MOD_EB, "Get flash part %d size failed, valid index[0 - %d] ",
               part_id, SD_CARD_PART_NUM - 1);
    return -1;
  }
  char part_name[128] = {0};
  snprintf(part_name, 128, (char *)FLASH_PART_PATH_HEAD, part_id + 1);
  return GetPartInfo(part_name, info);
#else
  return 0;
#endif
}

int DiskHelper::GetFlashMaxPartSize(PartitionInfo *info) {
#ifndef _WIN32
  if (NULL == info) {
    DLOG_ERROR(MOD_EB, "Get flash partition size faile, invalid para");
    return -1;
  }

  int32 max_part_idx = -1;
  uint32 max_free_size = 0;
  char part_name[128] = {0};
  for (int part_idx = 0; part_idx < SD_CARD_PART_NUM; part_idx++) {
    PartitionInfo part_info;
    snprintf(part_name, 128, (char *)FLASH_PART_PATH_HEAD, part_idx + 1);
    int ret = GetPartInfo(part_name, &part_info);
    if (0 == ret) {
      if (part_info.free > max_free_size) {
        max_part_idx = part_idx;
        max_free_size = part_info.free;
        info->total = part_info.total;
        info->used = part_info.used;
        info->free = part_info.free;
      }
    }
  }
  return max_part_idx;
#else
  return 0;
#endif
}

unsigned int DiskHelper::GetFlashCurPart(uint32 size_limit, PartitionInfo *cur_part) {
  // to-do:
  // A\B分区.
  //  A分区used=0,B分区free>limit.当前分区=B
  //  A分区used=0,B分区free<=limit.当前分区=A
  //  A分区free<limit,B分区free>limit.当前分区=B
  //  A分区free>limit,B分区free<limit.当前分区=A

#ifndef _WIN32
#ifdef MULTI_PART_STORAGE
  PartitionInfo info[SD_CARD_PART_NUM];
  GetFlashPartSize(0, &info[0]);
  GetFlashPartSize(1, &info[1]);

  int cur_idx = 0;
  if (info[0].used == 0 && info[1].free > size_limit) {
    cur_idx = 1;
  } else if (info[0].used == 0 && info[1].free <= size_limit) {
    cur_idx = 0;
  } else if (info[0].free > size_limit && info[1].used == 0) {
    cur_idx = 0;
  } else if (info[0].free <= size_limit && info[1].used == 0) {
    cur_idx = 1;
  } else if (info[0].free < size_limit && info[1].free > size_limit) {
    cur_idx = 1;
  } else if (info[0].free > size_limit && info[1].free < size_limit) {
    cur_idx = 0;
  }

  if (PART_ERROR != part_info_.nand_part[cur_idx].flag) {
    if (cur_part) {
      memcpy(cur_part, &info[cur_idx], sizeof(PartitionInfo));
    }
    return cur_idx;
  }

  // 分区不可用, 切换分区
  cur_idx = (cur_idx == 0) ? 1 : 0;
  if (PART_ERROR != part_info_.nand_part[cur_idx].flag) {
    if (cur_part) {
      memcpy(cur_part, &info[cur_idx], sizeof(PartitionInfo));
    }
    return cur_idx;
  }

  memset(cur_part, 0, sizeof(PartitionInfo));
  return (unsigned int)-1;  // 磁盘损坏
#else
  if (cur_part) {
    GetSDCardCurPart(0, cur_part);
  }
  return 0;
#endif
#endif
  return 0;
}

int DiskHelper::GetSDCardPartSize(uint32 part_id, PartitionInfo *info) {
#ifndef _WIN32
  if (NULL == info) {
    DLOG_ERROR(MOD_EB, "Get SD card partition size faile, invalid para");
    return -1;
  }
  if (part_id >= SD_CARD_PART_NUM) {
    DLOG_ERROR(MOD_EB, "Get SD card part %d size failed, valid index[0 - %d] ",
               part_id, SD_CARD_PART_NUM - 1);
    return -1;
  }

  char part_name[128] = {0};
  snprintf(part_name, 128, (char *)CARD_PART_PATH_HEAD, part_id + 1);
  return GetPartInfo(part_name, info);
#else
  return 0;
#endif
}

int DiskHelper::GetSDCardMaxPartSize(PartitionInfo *info) {
#ifndef _WIN32
  if (NULL == info) {
    DLOG_ERROR(MOD_EB, "Get SD card partition size faile, invalid para");
    return -1;
  }

  int32 max_part_idx = -1;
  uint32 max_free_size = 0;
  for (int part_idx = 0; part_idx < SD_CARD_PART_NUM; part_idx++) {
    PartitionInfo part_info;
    int ret =
      GetPartInfo(part_info_.card_part[part_idx].mount_path, &part_info);
    if (0 == ret) {
      if (part_info.free > max_free_size) {
        max_part_idx = part_idx;
        max_free_size = part_info.free;
        info->total = part_info.total;
        info->used = part_info.used;
        info->free = part_info.free;
      }
    }
  }
  return max_part_idx;
#else
  return 0;
#endif
}

unsigned int DiskHelper::GetSDCardCurPart(uint32 size_limit, PartitionInfo *cur_part) {
  // A\B分区.
  //  A分区used=0,B分区free>limit.当前分区=B
  //  A分区used=0,B分区free<=limit.当前分区=A
  //  A分区free<limit,B分区free>limit.当前分区=B
  //  A分区free>limit,B分区free<limit.当前分区=A

#ifndef _WIN32
#ifdef MULTI_PART_STORAGE
  PartitionInfo info[SD_CARD_PART_NUM];
  GetSDCardPartSize(0, &info[0]);
  GetSDCardPartSize(1, &info[1]);

  int cur_idx = 0;
  if (info[0].used == 0 && info[1].free > size_limit) {
    cur_idx = 1;
  } else if (info[0].used == 0 && info[1].free <= size_limit) {
    cur_idx = 0;
  } else if (info[0].free > size_limit && info[1].used == 0) {
    cur_idx = 0;
  } else if (info[0].free <= size_limit && info[1].used == 0) {
    cur_idx = 1;
  } else if (info[0].free < size_limit && info[1].free > size_limit) {
    cur_idx = 1;
  } else if (info[0].free > size_limit && info[1].free < size_limit) {
    cur_idx = 0;
  }

  if (PART_MOUNTED == part_info_.card_part[cur_idx].flag) {
    if (cur_part) {
      memcpy(cur_part, &info[cur_idx], sizeof(PartitionInfo));
    }
    return cur_idx;
  }

  // 分区不可用, 切换分区
  cur_idx = (cur_idx == 0) ? 1 : 0;
  if (PART_MOUNTED == part_info_.card_part[cur_idx].flag) {
    if (cur_part) {
      memcpy(cur_part, &info[cur_idx], sizeof(PartitionInfo));
    }
    return cur_idx;
  }

  memset(cur_part, 0, sizeof(PartitionInfo));
  return (unsigned int)-1;  // 磁盘损坏
#else
  if (cur_part) {
    GetSDCardCurPart(0, cur_part);
  }
  return 0;
#endif
#endif
  return 0;
}

void DiskHelper::InformError(bool is_sd, uint32 idx, bool is_err) {
#ifndef _WIN32
  if (idx >= SD_CARD_PART_NUM) {
    return;
  }

  int ret = 0;
  if (is_sd) {
    if (is_err) {
      UnMountSDCard(idx);
      part_info_.card_part[idx].flag = PART_ERROR;
    } else {
      part_info_.card_part[idx].flag = PART_MOUNTED;
    }
  } else {
    if (is_err) {
      part_info_.nand_part[idx].flag = PART_ERROR;
    } else {
      part_info_.nand_part[idx].flag = PART_MOUNTED;
    }
  }
#endif
}

int DiskHelper::GetBlockDeviceInfo(VzBlockDeviceInfo *dev_info) {
#ifndef _WIN32
  if (NULL == dev_info) {
    DLOG_ERROR(MOD_EB, "Get block device failed, invalid para");
    return -1;
  }
  vzes::CritScope cs(&crit_);
  VzBlockDeviceInfo *info = NULL;
  info = VZ_DeviceSDK_BlockDevice_GetInfo();
  if (!info) {
    DLOG_ERROR(MOD_EB, "Get block device info failed");
    return -1;
  }
  memcpy(dev_info, info, sizeof(VzBlockDeviceInfo));

  for (int dev_idx = 0; dev_idx < dev_info->BlockDevNum; dev_idx++) {
    if (BLOCK_DEVICE_SD == dev_info->BlockDev[dev_idx].BlockDevFlag) {
      VzBlockDevice &sd_part_info = dev_info->BlockDev[dev_idx];

      for (int part_idx = 0; part_idx < sd_part_info.PartNum; part_idx++) {
        if (0 == strcmp(
              part_info_.card_part[part_idx].dev_path,
              sd_part_info.PartInfo[part_idx].PartDevPath)) {
          if (PART_ERROR == part_info_.card_part[part_idx].flag) {
            sd_part_info.PartInfo[part_idx].PartFlag = PART_ERROR;
          }
        }
      }

      break;
    }
  }
#endif
  return 0;
}

// #endif

bool DiskHelper::is_file_exist(const char *filepath) {
#ifndef _WIN32
  if (0 == access(filepath, 0)) {
    return true;
  }
#endif
  return false;
}

int DiskHelper::GetPartInfo(const char *dir, PartitionInfo *part_info) {
#ifndef _WIN32
  if (NULL == dir || NULL == part_info) {
    DLOG_ERROR(MOD_EB, "Get part info failed, invalid para");
    return -1;
  }
#ifndef _WIN32
  struct statfs disk_stat;
  if (statfs(dir, &disk_stat) == 0) {
    // printf("disk_statfs.f_blocks = %d\n", disk_statfs.f_blocks);
    // printf("disk_statfs.f_bfree = %d\n", disk_statfs.f_bfree);
    // printf("disk_statfs.f_bsize = %d\n", disk_statfs.f_bsize);
    // printf("disk_statfs.f_files = %d\n", disk_statfs.f_files);
    // printf("disk_statfs.f_ffree = %d\n", disk_statfs.f_ffree);

    part_info->free =
      ((long long)disk_stat.f_bsize * disk_stat.f_bfree) / 1000000;
    part_info->total =
      ((long long)(disk_stat.f_bsize) * disk_stat.f_blocks) / 1000000;
    part_info->used = part_info->total - part_info->free;
    return 0;
  }
#else
  part_info->total = 8000;
  part_info->used = 1000;
  part_info->free = part_info->total - part_info->used;
  return 0;
#endif
#endif
  return -1;
}

int DiskHelper::GetABSFilePath(const char *filename, char path[128]) {
#ifndef _WIN32
  if (NULL == filename || NULL == path) {
    DLOG_ERROR(MOD_EB, "Get part info failed, invalid para");
    return -1;
  }

  int pathsize = 0;
  for (int part_idx = 0; part_idx < SD_CARD_PART_NUM; part_idx++) {
    // NAND
#ifdef _WIN32
    pathsize = snprintf(path, 127, "c:\\vz_file_cache\\%s", filename);
#else
    pathsize =
      snprintf(path, 127, FLASH_PART_ABS_PATH, (part_idx + 1), filename);
#endif
    path[pathsize] = '\0';
    if (is_file_exist(path)) {
      return 0;
    }

    // SD
#ifdef _WIN32
    pathsize = snprintf(path, 127, "c:\\vz_file_cache\\%s", filename);
#else
    pathsize =
      snprintf(path, 127, CARD_PART_ABS_PATH, (part_idx + 1), filename);
#endif
    path[pathsize] = '\0';
    if (is_file_exist(path)) {
      return 0;
    }
  }

  if (is_file_exist(filename)) {
    strncpy(path, filename, MAX_PATH_SIZE);
    return 0;
  }

  path[0] = '\0';
#endif
  return -1;
}
}  // namespace vzes
