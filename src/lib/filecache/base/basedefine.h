//


#ifndef FILECACHE_BASE_BASE_DEFINE_H_
#define FILECACHE_BASE_BASE_DEFINE_H_

#include "eventservice/mem/membuffer.h"

namespace cache {

#define VZ_RECORD_PART_MAX            2   // ����������
#define VZ_RECORD_PATH_SIZE           128
#define VZ_RECORD_PATH                "VzIPCCap"  // ��ʶ�洢Ŀ¼


typedef vzes::MemBuffer  MemBuffer;

#define CACHE_ERROR_FILE_NOT_FOUND    1
#define CACHE_ERROR_TIMEOUT           2
#define CACHE_ERROR_NO_MEM            3
#define CACHE_DONE                    0

#define DEFAULT_REQ_TIMEOUT           3000

}


#endif  // FILECACHE_BASE_BASE_DEFINE_H_
