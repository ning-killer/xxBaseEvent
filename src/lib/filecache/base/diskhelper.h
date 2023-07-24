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
#ifndef _VZBASE_DISK_HELPER_H_
#define _VZBASE_DISK_HELPER_H_

#include "eventservice/base/basictypes.h"
#include "eventservice/net/eventservice.h"
//#if !defined(WIN32) && !defined(UBUNTU64)
#include "VzSdk/VzDeviceSDK.h"
//#endif

namespace vzes {

// 存储设备分区数量、挂载路径定义
#ifdef MULTI_PART_STORAGE
#define SD_CARD_PART_NUM        (2)
#ifdef LITEOS
// Liteos系统挂载到同一个目录下概率异常
#define FLASH_PART_PATH_HEAD    "/media_nand%d"
#define CARD_PART_PATH_HEAD     "/media_card%d"
#else
#define FLASH_PART_PATH_HEAD    "/media/media_nand%d"
#define FLASH_PART_ABS_PATH     "/media/media_nand%d/%s"
#define CARD_PART_PATH_HEAD     "/media/media_card%d"
#define CARD_PART_ABS_PATH      "/media/media_card%d/%s"
#endif
#else
#define SD_CARD_PART_NUM        (1)
#define FLASH_PART_PATH_HEAD    "/media/mmcblk0p%d"
#define FLASH_PART_ABS_PATH     "/media/media_nand%d/%s"
#define CARD_PART_PATH_HEAD    	"/media/mmcblk0p%d"
#define CARD_PART_ABS_PATH      "/media/media_card%d/%s"
#endif

#ifndef MAX_PATH_SIZE
#define MAX_PATH_SIZE           (128)
#endif

#ifndef DISK_PATH_SIZE
#define DISK_PATH_SIZE          (36)
#endif  // DISK_PATH_SIZE

class DiskHelper : public MessageHandler,
  public boost::enable_shared_from_this<DiskHelper> {
 public:
  typedef boost::shared_ptr<DiskHelper> Ptr;

  // 创建DiskHelper实例，该类为单实例类，应用应通过该接口获取类实例
  static DiskHelper* Instance(EventService::Ptr es = EventService::Ptr());
  DiskHelper(EventService::Ptr es);
  virtual ~DiskHelper();

  void OnMessage(vzes::Message* message);

  unsigned int SD_PART_NUM();

// #if !defined(WIN32) && !defined(UBUNTU64)
  // 挂载SD卡，同步挂载；如果未分区则先分区后，异步挂载。
  // return 成功:0; 失败:-1
  int  MountSDCard(void);

  // 卸载SD卡
  // return 成功:0; 失败:-1
  int UnMountSDCard(uint32 part_id);

  // 读取SD Card当前挂载状态，如果当前未挂载，会触发异步挂载请求
  // return 当前SD卡的挂载状态，已挂载: true
  bool IsSDCardMounted(void);

  // 格式化SD卡，指定的分区
  // part_id: 分区索引，[0, SD_CARD_PART_NUM-1]
  // return 成功:0; 失败:-1
  int  FormatSDCardPartition(uint32 part_id);
  int  FormatSDCardPartition(vzstd::string dev_path);

  // SD卡分区
  // return 成功:0; 失败:-1
  int SplitPartSDCard(void);

  // 格式化flash，指定的分区
  // part_id: 分区索引，[0, SD_CARD_PART_NUM-1]
  // return 成功:0; 失败:-1
  int FormatFlashPartition(uint32 part_id);

  // 格式化flash，所有的分区
  // return 成功:0; 失败:-1
  int  FormatFlash(void);

  // 读取Flash分区大小,指定的分区
  // part_id: 分区索引，[0, SD_CARD_PART_NUM-1]
  // info: 分区信息，单位MB，用户自己管理内存
  // return 成功:0; 失败:-1
  int GetFlashPartSize(uint32 part_id, PartitionInfo *info);

  // 读取Flash Free空间最大的分区信息
  // info: 分区信息，单位MB，用户自己管理内存
  // return 成功:>=0,分区index，[0, SD_CARD_PART_NUM-1]; 失败:-1
  int GetFlashMaxPartSize(PartitionInfo *info);
  unsigned int GetFlashCurPart(uint32 size_limit, PartitionInfo *cur_part);

  // 读取SD卡分区大小,指定的分区
  // part_id: 分区索引，[0, SD_CARD_PART_NUM-1]
  // info: 分区信息，单位MB，用户自己管理内存
  // return 成功:0; 失败:-1
  int GetSDCardPartSize(uint32 part_id, PartitionInfo *info);

  // 读取SD卡Free空间最大的分区信息
  // info: 分区信息，单位MB，用户自己管理内存
  // return 成功:>=0,分区index，[0, SD_CARD_PART_NUM-1]; 失败:-1
  int GetSDCardMaxPartSize(PartitionInfo *info);

  unsigned int GetSDCardCurPart(uint32 size_limit,
                                PartitionInfo *cur_part);

  // 设置异常状态
  // err_card:是否为错卡
  void InformError(bool is_sd, uint32 idx, bool is_err);

  // 读取块设备信息
  // dev_info: [OUT]设备信息
  // return 成功:0; 失败:-1
  int GetBlockDeviceInfo(VzBlockDeviceInfo *dev_info);

  // return: true or false
  static bool is_file_exist(const char *filepath);
  // return: 0=success; -1=failed
  static int GetPartInfo(const char *dir, PartitionInfo *part_info);
  static int GetABSFilePath(const char *filename, char path[128]);

 private:
  void AsyncCheckSDCard(void);

 private:
  vzes::CriticalSection    crit_;
// #endif

  vzes::EventService::Ptr  event_service_;
  static DiskHelper       *instance_;
  bool                     async_mounting_;

  typedef struct {
    unsigned int flag;
    char dev_path[VZ_BLOCK_DEVICE_PATH_SIZE];
    char mount_path[VZ_BLOCK_DEVICE_PATH_SIZE];
  } TAG_PART_INFO;
  struct {
    TAG_PART_INFO nand_part[SD_CARD_PART_NUM];
    TAG_PART_INFO card_part[SD_CARD_PART_NUM];
  } part_info_;
}; // class DiskHelper

} // namespace vzes


#endif  // _VZBASE_DISK_HELPER_H_
