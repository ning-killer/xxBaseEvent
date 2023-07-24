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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#ifdef POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#ifndef LITEOS
#include <sys/ipc.h>
#endif
#endif
#include "eventservice/base/basictypes.h"
#include "eventservice/base/common.h"
#include "eventservice/base/atomic.h"
#include "eventservice/base/timeutils.h"
#include "log/log/log_cache.h"
#include "log/log/log_client.h"
#include "astl/mem_dump.h"


// 内存栈索引，占高16位, 位移，低位为16或48位
#define MEM_STACK_TOP_OFFSET     ((sizeof(ulong) - sizeof(uint16)) << 3)
// 引用计数，占剩余低位, 掩码/
#define MEM_STACK_REF_MASK       (((ulong)1 << (MEM_STACK_TOP_OFFSET)) - 1)
// 内存栈尾节点next指针值，无效值/
#define MEM_STACK_NODE_INVALID   (0xFFFF)
// 数据块 尾部填充 长度
#define MEM_STACK_DATA_PADDING   (sizeof(ulong))
// 数据块 尾部填充 值
#define MEM_STACK_DATA_MAGIC     ((ulong)0x12345678)
// Log Cache SHM KEY
#define LOG_CACHE_SHM_KEY        (10010)


// 索引队列中结点准备状态
typedef enum {
  NODE_EMPTY = 0,                  // 消息块初始状态
  NODE_READY,                      // 消息块已经准备好
  NODE_INVALID,                    // 消息块处于无效状态
} IDX_QUEUE_NODE_STATE_E;

// 内存栈节点信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
//#define MEM_STACK_NODE_PACK (sizeof(ulong) - sizeof(long) - sizeof(long))
typedef struct {
  volatile long   usNext;          // 指向下一个数据块的索引号
  volatile ulong  bUsedFlag;       // 标示该内存块是否在用
} MEM_STACK_NODE_S;

// 内存栈信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
typedef struct {
  uint16          usDataBlockCnt;  // 内存栈中节点个数
  uint16          usDataBlockSize; // 内存栈中节点尺寸，用户配置的Size + 尾部Pad:MEM_STACK_DATA_MAGIC
//uint8           aucReserved1[4]; // 64位系统8字节对齐
  volatile ulong  usUsedCnt;       // 内存栈中已经使用的节点个数
  volatile ulong  usUsedCntMax;    // 内存栈中历史使用的节点最大个数
  volatile ulong  ulTopAndRef;     // 高字16位-栈结构顶部结点序号,第一个可用的节点index;
  // 低16 or 48位-出栈次数，二者组合用于实现并发互斥
  MEM_STACK_NODE_S astNode[0];     // 队列存储区:索引数组
} MEM_STACK_S;

// 索引队列节点信息结构体:该结构32位系统4字节对齐、64位系统均8字节对齐
#define IDX_QUEUE_NODE_PACK (sizeof(ulong) - sizeof(uint16) - sizeof(uint8))
typedef struct {
  volatile long   ucState;         // 索引块的准备状态:NODE_EMPTY,NODE_READY
  uint16          usDataIdx;       // 内存栈中的节点索引号
  uint8           ucStackIdx;      // 内存栈索引号
  uint8           aucReserved[IDX_QUEUE_NODE_PACK];
} IDX_QUEUE_NODE_S;

// Log Cache实体、索引队列信息结构体
// 该结构32位系统4字节对齐、64位系统均8字节对齐
typedef struct {
  int             iServerMode;     // Server模式标识，Server模式下内存为SHM
  int             iShmId;          // Server模式SHM ID
  int             pid;             // 创建LogEntity进程的PID
  uint32          uiMemSize;       // Log Cache整个内存的大小
  uint16          usIdxBlockCnt;   // 索引队列数据块个数
  uint16          usIdxBlockSize;  // 索引队列数据块尺寸
//uint8           aucReserved1[4]; // 64位系统8字节对齐
  volatile ulong  ulEnQCnt;        // 索引队列执行入队列次数，通过此来获取tail位置
  volatile ulong  usHead;          // 索引队列中第一个可用结点索引
  volatile ulong  usMsgCnt;        // 索引队列可用消息结点个数
  uint8           ucDStackCnt;     // 内存栈子栈的个数
  uint8           aucReserved[3];  // 占位，保证结构为4 or 8字节对接

  uint8           ucBody[0];       // 占位，mem stack偏移索引段 + 索引队列 + mem stack 1~n
  //ulong         aulOffset[0];    // mem stack偏移索引段起始地址，方便数据访问
} IDX_QUEUE_S;

IDX_QUEUE_S  *g_pstPQueue = NULL;


// 查询共享内存，如果系统中已存在该共享内存，则映射到当前进程。
void* ShmGet(int key, int *id) {
#if defined(WIN32) || defined(LITEOS)
  return NULL;
#else
  int shm_id = -1;
  void *shm_mem = NULL;
  shm_id = shmget((key_t)key, 0, 0);
  if (-1 == shm_id) {
    printf("[LogCache]:Get SHM failed, key:%d, err:%s\n", key, strerror(errno));
    return NULL;
  }
  shm_mem = shmat(shm_id, (void*)0, 0);
  if (shm_mem == (void*)-1) {
    printf("[LogCache]:Attach SHM failed, key:%d, err:%s\n", key, strerror(errno));
    (void)shmctl(shm_id, SHM_UNLOCK, NULL);
    return NULL;
  }
  *id = shm_id;
  return shm_mem;
#endif
}

// 关闭共享内存，取消对该共享内存的映射，并删除（当没有进程attach到
// 该内存时系统才会真正的删除该内存）
void ShmClose(int iShmId, void *pvShmMem) {
#if defined(WIN32) || defined(LITEOS)
  return;
#else
  int res = shmctl(iShmId, IPC_RMID, NULL);
  if (0 != res) {
    printf("[LogCache]:Remove SHM failed, err:%s\n", strerror(errno));
  }
  res = shmdt(pvShmMem);
  if (0 != res) {
    printf("[LogCache]:Detach SHM failed, err:%s\n", strerror(errno));
  }
#endif
}

// 创建共享内存，并映射到当前进程。
void* ShmCreate(uint32 size, int *id) {
#if defined(WIN32) || defined(LITEOS)
  return NULL;
#else
  int shm_id = -1;
  void *shm_mem = NULL;
  do {
    shm_id = shmget((key_t)LOG_CACHE_SHM_KEY, size, 0666|IPC_CREAT);
    if (-1 == shm_id) {
      printf("[LogCache]:Create SHM failed, err:%s\n", strerror(errno));
      break;
    }
    if (-1 == shmctl(shm_id, SHM_LOCK, NULL)) {
      printf("[LogCache]:Lock SHM failed, err:%s\n", strerror(errno));
      break;
    }
    shm_mem = shmat(shm_id, (void*)0, 0);
    if (shm_mem == (void*)-1) {
      printf("[LogCache]:Attach SHM failed, err:%s\n", strerror(errno));
      (void)shmctl(shm_id, SHM_UNLOCK, NULL);
      break;
    }
    *id = shm_id;
    printf("[LogCache]:Create SHM successed, key:%d,id:%d,size:%d\n",
           LOG_CACHE_SHM_KEY, shm_id, size);
    return shm_mem;
  } while(0);

  if (-1 != shm_id) {
    (void)shmctl(shm_id, IPC_RMID, NULL);
  }
  printf("[LogCache]:Create SHM failed\n");
  return NULL;
#endif
}

// 获取当前进程的PID
int GetSelfPid(void) {
  int pid = 0;
#ifdef WIN32
#elif defined LITEOS
#else
  pid = (int)getpid();
#endif
  return pid;
}

// 设置日志节点时间
void SetTime(void *pvMem) {
  LOG_NODE_S *cache = (LOG_NODE_S*)pvMem;
  if (NULL == cache) {
    return;
  }

#ifdef WIN32
  // windows下无法获取精确的usec，每次递增1微秒来区分时间，
  // 但概率存在时间反转
  static uint32 offset = 0;
  offset = (offset + 1) & (31);
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  cache->sec = tv.sec;
  cache->usec = tv.usec + offset;
#else
  vzes::TimeVal tv;
  vzes::TimeOfDay(&tv, NULL);
  cache->sec = tv.sec;
  cache->usec = tv.usec;
#endif
}

// 获取索引队列首地址
// pstPQueue:Log Cache头指针
// return 索引队列首地址
static inline IDX_QUEUE_NODE_S* IdxQueueHead(IDX_QUEUE_S *pstPQueue) {
  return (IDX_QUEUE_NODE_S*)((ulong*)pstPQueue->ucBody + pstPQueue->ucDStackCnt);
  //return (IDX_QUEUE_NODE_S*)&pstPQueue->aulOffset[pstPQueue->ucDStackCnt];
}

// 索引队列节点地址
// pstHead:索引队列首地址
// usIndex:节点下标
// return 节点的地址
static inline IDX_QUEUE_NODE_S* IdxQueueNode(IDX_QUEUE_NODE_S *pstHead,
    uint16 uiIndex) {
  return &pstHead[uiIndex];
}

// 根据下标取内存栈的偏移
// pstPQueue:Log Cache头指针
// usIndex:内存栈索引
// return 内存栈的偏移量
static inline uint32 MemStackOffset(IDX_QUEUE_S *pstPQueue,
                                    uint16 usIndex) {
  return *(((ulong*)pstPQueue->ucBody) + usIndex);
  //return pstPQueue->aulOffset[usIndex];
}

// 根据下标取内存栈首地址
// pstPQueue:Log Cache头指针
// usIndex:内存栈索引
// return 内存栈首地址
static inline MEM_STACK_S* MemStackHead(IDX_QUEUE_S *pstPQueue,
                                        uint16 usIndex) {
  return (MEM_STACK_S*)&pstPQueue->ucBody[MemStackOffset(pstPQueue, usIndex)];
}

// 根据下标取内存栈节点地址
// pstStack:内存栈指针
// usIndex:栈节点下标
// return 节点地址
static inline MEM_STACK_NODE_S* MemStackNode(MEM_STACK_S *pstStack,
    ulong usIndex) {
  return &pstStack->astNode[usIndex];
}

// 根据下标取内存栈节点数据区首地址
// pstStack:内存栈指针
// usIndex:栈节点下标
// return 节点数据区首地址
static inline void* MemStackData(MEM_STACK_S *pstStack, ulong usIndex) {
  uint8 *pucData = (uint8*)&pstStack->astNode[pstStack->usDataBlockCnt];
  return &pucData[pstStack->usDataBlockSize * usIndex];
}

// 取栈数据区总长
// pstStack:内存栈指针
// return 数据区总长
static inline uint32 MemStackSize(MEM_STACK_S *pstStack) {
  return pstStack->usDataBlockCnt * pstStack->usDataBlockSize;
}

// 检查内存是否覆盖 Padding填充区
// pstStack:内存栈指针
// usIndex:内存块下标
// pvData:内存块首地址
static void MemStackNodeCheck(MEM_STACK_S *pstStack, uint16 usIndex,
                              void* pvData) {
  uint8  *pucData = (uint8*)pvData;
  ulong  *pulPad  = NULL;

  pulPad = (ulong*)&pucData[pstStack->usDataBlockSize - MEM_STACK_DATA_PADDING];
  if (*pulPad == MEM_STACK_DATA_MAGIC) {
    return;
  }

  printf("[logCache]error: pvData:%p, corrupts pad!!!\n", pvData);
}

void Log_CacheDump(uint8 ucType, int16 sDStackIdx) {
  uint8   *pucBuf  = NULL;
  uint32   uiSize  = 1024;
  int      iOffset = 0;
  uint16   i       = 0;

  if (NULL == g_pstPQueue) {
    return;
  }

  pucBuf = (uint8*)VZ_MALLOC(uiSize);
  if (NULL == pucBuf) {
    return;
  }

  // dump queue header info
  if (ucType & LOG_CACHE_DUMP_HEAD) {
    printf("PQueue(%p): \r\n\tidx-cnt:%d \r\n\tidx-size:%d \r\n\thead:%d \r\n\t"
           "tail:%lu \r\n\tenqcnt:%lu \r\n\tmsg-cnt:%d \r\n\tds-cnt:%d\n",
           g_pstPQueue,
           g_pstPQueue->usIdxBlockCnt,
           g_pstPQueue->usIdxBlockSize,
           g_pstPQueue->usHead,
           g_pstPQueue->ulEnQCnt % g_pstPQueue->usIdxBlockCnt,
           g_pstPQueue->ulEnQCnt,
           g_pstPQueue->usMsgCnt,
           g_pstPQueue->ucDStackCnt);
  }

  // dump data stack address
  if (ucType & LOG_CACHE_DUMP_OFFSET) {
    iOffset = snprintf((char*)pucBuf, uiSize,
                       "PQueue-Offsets(%p):\r\n\t", g_pstPQueue->ucBody);
    for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
      iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "0x%x ",
                          MemStackOffset(g_pstPQueue, i));
    }
    printf("%s", pucBuf);
  }

  // dump index array info
  if (ucType & LOG_CACHE_DUMP_IDX) {
    IDX_QUEUE_NODE_S  *pstHdr;
    pstHdr  = IdxQueueHead(g_pstPQueue);
    iOffset = snprintf((char*)pucBuf, uiSize, "PQueue-Idx(%p):\r\n\t", pstHdr);
    for (i=0; i<g_pstPQueue->usIdxBlockCnt; i++) {
      IDX_QUEUE_NODE_S  *pstIdx;
      pstIdx = IdxQueueNode(pstHdr, i);
      iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset,
                          "stack-idx:%d, blk-idx:%d, state:%d",
                          pstIdx->ucStackIdx, pstIdx->usDataIdx, (int)pstIdx->ucState);
      if (((i+1) % 25) != 0) {
        iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "\r\n\t");
        continue;
      }

      printf("%s", pucBuf);
      pucBuf[0] = '\t';
      pucBuf[1] = '\0';
      iOffset = 1;
    }

    if (((i+1) % 25) != 0) {
      *(pucBuf + iOffset - 3) = '\0';
      printf("%s", pucBuf);
    }
  }

  // dump all data stack header info or the stack header which id is ucPQueueDIdx
  if (ucType & LOG_CACHE_DUMP_DSTACK) {
    MEM_STACK_S  *pstDStack;
    if (-1 == sDStackIdx) {
      iOffset = 0;
      // data stack infos
      for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
        pstDStack = MemStackHead(g_pstPQueue, i);
        iOffset  += snprintf((char*)pucBuf + iOffset, uiSize - iOffset,
                             "DStack%d(%p): \r\n\tblk-cnt:%d, blk-size:%d, "
                             "topandref:%lu, use-cnt:%d, use-max:%d",
                             i,
                             pstDStack,
                             pstDStack->usDataBlockCnt,
                             pstDStack->usDataBlockSize,
                             pstDStack->ulTopAndRef,
                             pstDStack->usUsedCnt,
                             pstDStack->usUsedCntMax);
        if (((i+1) % 15) != 0) {
          iOffset += snprintf((char*)pucBuf + iOffset, uiSize - iOffset, "\r\n");
          continue;
        }

        printf("%s\n", pucBuf);
        pucBuf[0] = '\t';
        pucBuf[1] = '\0';
        iOffset = 1;
      }

      if (((i+1) % 15) != 0) {
        *(pucBuf + iOffset - 2) = '\0';
        printf("%s", pucBuf);
      }
    } else {
      if ((0 <= sDStackIdx) && (sDStackIdx < g_pstPQueue->ucDStackCnt)) {
        pstDStack = MemStackHead(g_pstPQueue, sDStackIdx);
        printf("DStack%d(%p): \r\n\tblk-cnt:%d, blk-size:%d, topandref:%lu, "
               "use-cnt:%d, use-max:%d\n",
               i,
               pstDStack,
               pstDStack->usDataBlockCnt,
               pstDStack->usDataBlockSize,
               pstDStack->ulTopAndRef,
               pstDStack->usUsedCnt,
               pstDStack->usUsedCntMax);
      }
    }
  }

  if (pucBuf) {
    VZ_FREE(pucBuf);
  }
}

int Log_CacheAdd(void *pvMem) {
  IDX_QUEUE_NODE_S  *pstIdxNode;
  MEM_STACK_S       *pstDStack;
  void              *pvDStackTop;
  ulong              ulOld, ulNew;
  uint16             usTail, usNext, usDataIdx;
  uint8              ucStackIdx, i;

  //if ((NULL == g_pstPQueue) || (NULL == pvMem)) {
  //  printf("[logCache]error:Invalid args!(queue:%p,msg:%p)\n", g_pstPQueue, pvMem);
  //  return LOG_ERROR;
  //}

  // 检查待添加的内存是否在内存栈中
  pstDStack  = NULL;
  for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
    ucStackIdx  = i;
    pstDStack   = MemStackHead(g_pstPQueue, i);
    pvDStackTop = MemStackData(pstDStack, 0);
    if (((void *)pvMem >= pvDStackTop)
        && ((uint8*)pvMem < ((uint8*)pvDStackTop + MemStackSize(pstDStack)))
        && ((((uint8*)pvMem - (uint8*)pvDStackTop) % pstDStack->usDataBlockSize) == 0)) {
      break;
    }
  }

  if (i == g_pstPQueue->ucDStackCnt) {
    printf("[logCache]error:Invalid args!(msg:%p)\n", pvMem);
    return LOG_ERROR;
  }

  // 将上一步存储好的数据头指针填入索引队列中去
  do {
    ulOld  = g_pstPQueue->ulEnQCnt;
    ulNew  = ulOld + 1;
    usTail = ulOld % g_pstPQueue->usIdxBlockCnt;
    if (ulNew == 0) {
      // ulEnQCnt计数溢出(等于0)，从当前tail计算出下一个tail位置
      usNext = (usTail + 1) % g_pstPQueue->usIdxBlockCnt;
      ulNew  = usNext;
      printf("[logCache]enQueue overflow!(enqcnt:%d,msg-c:%d,blk-c:%d,old:%d,"
             "next:%d,head:%d,tail:%d)\n",
             g_pstPQueue->ulEnQCnt, g_pstPQueue->usMsgCnt, g_pstPQueue->usIdxBlockCnt,
             ulOld, usNext, g_pstPQueue->usHead, usTail);
    } else {
      usNext = ulNew % g_pstPQueue->usIdxBlockCnt;
    }

    if ((usNext == g_pstPQueue->usHead) && (g_pstPQueue->ulEnQCnt == ulOld)) {
      printf("[logCache]Index queue is full!(enqcnt:0x%x,blk-c:%d,msg-c:%d,"
             "stack-c:%d,head:%d,tail:%d)\n",
             g_pstPQueue->ulEnQCnt, g_pstPQueue->usIdxBlockCnt, g_pstPQueue->usMsgCnt,
             g_pstPQueue->ucDStackCnt, g_pstPQueue->usHead, usTail);
      return LOG_ERROR;
    }

    SetTime(pvMem);
  } while (!VZ_BCAS(&(g_pstPQueue->ulEnQCnt), ulOld, ulNew));

  // 记录当前队列中节点个数
  (void)VZ_FAA(&(g_pstPQueue->usMsgCnt), 1);

  // 将内存节点插入到索引队列
  usDataIdx  = (uint32)((uint8*)pvMem - (uint8*)pvDStackTop) / pstDStack->usDataBlockSize;
  pstIdxNode = IdxQueueNode(IdxQueueHead(g_pstPQueue), usTail);
  pstIdxNode->ucStackIdx = ucStackIdx;
  pstIdxNode->usDataIdx  = usDataIdx;
  (void)VZ_LTAS(&(pstIdxNode->ucState), NODE_READY); // 必须放在最后执行
  return LOG_OK;
}

void* Log_CacheGet(void) {
  IDX_QUEUE_NODE_S  *pstIdxNode;
  uint16             usTail, usNext;

  if (NULL == g_pstPQueue) {
    //printf("[logCache]error:invalid args!\n");
    return NULL;
  }

  usNext = (g_pstPQueue->usHead + 1) % g_pstPQueue->usIdxBlockCnt;
  usTail = g_pstPQueue->ulEnQCnt % g_pstPQueue->usIdxBlockCnt;
  if (usNext == usTail) {
    return NULL;
  }

  // 找到索引队列中第一个可用block,查看该结点是否已经ready
  pstIdxNode = IdxQueueNode(IdxQueueHead(g_pstPQueue), usNext);
  if (VZ_BCAS(&(pstIdxNode->ucState), NODE_READY, NODE_EMPTY)) {
    MEM_STACK_S *pstDStack = NULL;

    // 根据索引队列，确定内存块在内存栈中的位置
    pstDStack = MemStackHead(g_pstPQueue, pstIdxNode->ucStackIdx);

    // 从索引队列中将对应的结点清除
    (void)VZ_LTAS(&(g_pstPQueue->usHead), usNext);
    (void)VZ_FAS(&(g_pstPQueue->usMsgCnt), 1);
    return MemStackData(pstDStack, pstIdxNode->usDataIdx);
  }

  return NULL;
}

int Log_CacheCnt(void) {
  if (NULL == g_pstPQueue) {
    //printf("[logCache]error:invalid args!\n");
    return -1;
  }

  return g_pstPQueue->usMsgCnt;
}

int16 Log_CacheInfo(LOG_CACHE_INFO_S *pstPQueueInfo, uint8 *pucQueueCnt) {
  MEM_STACK_S  *pstDStack;
  uint8         ucQueueCnt, i;

  if ((NULL == g_pstPQueue)
      || (NULL == pucQueueCnt)
      || ((NULL != pstPQueueInfo) && (0 == *pucQueueCnt))) {
    printf("[logCache]error:invalid args!(queue:%p,info:%p,cnt:%p)\n",
           g_pstPQueue, pstPQueueInfo, pucQueueCnt);
    return -1;
  }

  ucQueueCnt   = *pucQueueCnt;
  *pucQueueCnt = g_pstPQueue->ucDStackCnt;
  if (NULL == pstPQueueInfo) {
    return 0;
  }

  for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
    if ((i+1) > ucQueueCnt) {
      break;
    }

    pstDStack = MemStackHead(g_pstPQueue, i);
    (pstPQueueInfo+i)->usBlockCnt        = pstDStack->usDataBlockCnt;
    (pstPQueueInfo+i)->usBlockSize       = pstDStack->usDataBlockSize - MEM_STACK_DATA_PADDING;
    (pstPQueueInfo+i)->usBlockCntUsed    = (uint16)pstDStack->usUsedCnt;
    (pstPQueueInfo+i)->usBlockCntUsedMax = (uint16)pstDStack->usUsedCntMax;
  }

  return i;
}

int Log_CacheFree(void *pvMem) {
  MEM_STACK_S      *pstDStack;
  MEM_STACK_NODE_S *pstDStackNode;
  void             *pvDStackTop;
  ulong             ulOld, ulNew;
  uint8             i;
  uint16            usIndex = 0;

  if ((NULL == g_pstPQueue) || (NULL == pvMem)) {
    printf("[logCache]error:invalid args!(pstPQ:%p, pMsg:%p)\n",
           g_pstPQueue, pvMem);
    return -1;
  }

  // 检查待释放的内存是否在内存栈的范围中
  if (((void*)g_pstPQueue > pvMem)
      || ((uint8*)pvMem > ((uint8*)g_pstPQueue + g_pstPQueue->uiMemSize))) {
    return -1;
  }

  // 查找待释放内存所属的内存栈
  pstDStack  = NULL;
  for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
    pstDStack   = MemStackHead(g_pstPQueue, i);
    pvDStackTop = MemStackData(pstDStack, 0);
    if ((pvMem >= pvDStackTop)
        && ((uint8*)pvMem < ((uint8*)pvDStackTop + MemStackSize(pstDStack)))
        && (((uint32)((uint8*)pvMem - (uint8*)pvDStackTop) % pstDStack->usDataBlockSize) == 0)) {
      usIndex = ((uint32)((uint8*)pvMem - (uint8*)pvDStackTop) / pstDStack->usDataBlockSize);
      break;
    }
  }

  if (NULL == pstDStack) {
    printf("[logCache]error:invalid args!(mem:%p)\n", pvMem);
    return -2;
  }

  // 检查内存块是否在被使用，如果内存块使用标志为true，那么将其置为false
  pstDStackNode = MemStackNode(pstDStack, usIndex);
  if (!VZ_BCAS(&(pstDStackNode->bUsedFlag), true, false)) {
    printf("[logCache]data block is not in use(mem:%p)", pvMem);
    return 0;
  }

  // 检查内存块是否有越界
  //MemStackNodeCheck(pstDStack, usIndex, pvMem);
  // 从索引队列中将内存栈节点对应的结点清除
  do {
    ulOld = pstDStack->ulTopAndRef;
    ulNew = usIndex;
    ulNew <<= MEM_STACK_TOP_OFFSET;
    ulNew |= ulOld & MEM_STACK_REF_MASK;
    pstDStackNode->usNext = ulOld >> MEM_STACK_TOP_OFFSET;
  } while(!VZ_BCAS(&(pstDStack->ulTopAndRef), ulOld, ulNew));

  // 记录当前内存栈中已分配的节点个数
  (void)VZ_FAS(&(pstDStack->usUsedCnt), 1);
  return 0;
}

void* Log_CacheMalloc(uint32 uiSize) {
  MEM_STACK_S  *pstDStack = NULL;
  void         *pvRet     = NULL;
  ulong         ulOld, ulNew, ulRefCnt;
  ulong         usTop, usUsedCnt;
  bool          bEqual = false;
  uint8         i;

  //if ((NULL == g_pstPQueue) || (0 == uiSize)) {
  //  printf("[logCache]error:invalid args!(queue:%p,size:%d)\n", g_pstPQueue, uiSize);
  //  return NULL;
  //}

  // 确定从哪个栈取内存块
  for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
    pstDStack = MemStackHead(g_pstPQueue, i);
    if (uiSize <= (pstDStack->usDataBlockSize - MEM_STACK_DATA_PADDING)) {
      break;
    }
    pstDStack = NULL;
  }

  if (NULL == pstDStack) {
    printf("[logCache]error:Malloc cache failed for over len, size:%d\n", uiSize);
    return NULL;
  }

  // 从内存栈中取出内存块
  do {
    ulOld    = pstDStack->ulTopAndRef;
    usTop    = ulOld >> MEM_STACK_TOP_OFFSET;
    ulRefCnt = ulOld & MEM_STACK_REF_MASK;

    // 检查内存栈是否还有空间
    if ((usTop == (uint16)MEM_STACK_NODE_INVALID)
        && (ulOld == pstDStack->ulTopAndRef)) {
      static uint32 count = 0;
      if (0 == (count % 128)) {
        printf("[logCache]memory stack full:\n");
        printf("stack | block size | block count | use count | max use count\n");
        for (i=0; i<g_pstPQueue->ucDStackCnt; i++) {
          MEM_STACK_S *pstDStackTmp = MemStackHead(g_pstPQueue, i);
          printf("%2d | %5d | %5d | %5d | %5d \n",
                 i, pstDStackTmp->usDataBlockSize - MEM_STACK_DATA_PADDING,
                 pstDStackTmp->usDataBlockCnt, pstDStackTmp->usUsedCnt,
                 pstDStackTmp->usUsedCntMax);
        }
      }
      count ++;
      return NULL;
    }

    ulNew   = MemStackNode(pstDStack, usTop)->usNext;
    ulNew <<= MEM_STACK_TOP_OFFSET;
    ulNew  |= (ulRefCnt + 1) & MEM_STACK_REF_MASK;
    //ulNew  = ((((ulong)ulNew) << MEM_STACK_TOP_OFFSET) &
    //         (~MEM_STACK_REF_MASK)) | ((ulRefCnt + 1) & MEM_STACK_REF_MASK);
  } while (!VZ_BCAS(&(pstDStack->ulTopAndRef), ulOld, ulNew));

  MemStackNode(pstDStack, usTop)->usNext = pstDStack->usDataBlockCnt; // MEM_STACK_NODE_INVALID;
  MemStackNode(pstDStack, usTop)->bUsedFlag = true;
  pvRet = MemStackData(pstDStack, usTop);

  // 记录当前栈中已分配节点个数，以及历史分配最大数量
  usUsedCnt = VZ_AAF(&(pstDStack->usUsedCnt), 1);
  if (usUsedCnt > pstDStack->usUsedCntMax) {
    (void)VZ_LTAS(&(pstDStack->usUsedCntMax), usUsedCnt);
  }

  return pvRet;
}

int Log_CacheReset(void) {
  if (!g_pstPQueue) {
    printf("[LogCache]:Reset Cache failed, not inited\n");
    return false;
  }

  printf("[LogCache]Reset Cache Start, TotalSize:%d, IdxBlockCnt:%d, "
         "IdxBlockSize:%d, DStackCnt:%d\n",
         g_pstPQueue->uiMemSize, g_pstPQueue->usIdxBlockCnt,
         g_pstPQueue->usIdxBlockSize, g_pstPQueue->ucDStackCnt);

  uint32 uiOffset = g_pstPQueue->ucDStackCnt * sizeof(ulong);
  uiOffset += g_pstPQueue->usIdxBlockCnt * sizeof(IDX_QUEUE_NODE_S);
  for (int i=0; i<g_pstPQueue->ucDStackCnt; i++) {
    // 栈.头
    MEM_STACK_S *pstDStack = (MEM_STACK_S*)&g_pstPQueue->ucBody[uiOffset];
    //*(((ulong*)g_pstPQueue->ucBody) + i) = uiOffset;
    //g_pstPQueue->aulOffset[i] = uiOffset;

    uint16 usDataBlockCnt = pstDStack->usDataBlockCnt;
    uint16 usDataBlockSize = pstDStack->usDataBlockSize;
    // 保证4字节对齐
    pstDStack->ulTopAndRef  = 0;
    pstDStack->usUsedCnt    = 0;
    pstDStack->usUsedCntMax = 0;
    uiOffset += sizeof(MEM_STACK_S);

    // 栈.索引节点 数组
    int j = 0;
    for (j = 0; j < usDataBlockCnt; j++) {
      pstDStack->astNode[j].usNext = j + 1;
    }
    pstDStack->astNode[j - 1].usNext = MEM_STACK_NODE_INVALID;
    uiOffset += sizeof(MEM_STACK_NODE_S) * usDataBlockCnt;
    uiOffset += usDataBlockSize * usDataBlockCnt;
  }

  g_pstPQueue->usHead   = 0; // head初始位置在0
  g_pstPQueue->ulEnQCnt = 1; // tail初始位置在1
  g_pstPQueue->usMsgCnt = 0;
  g_pstPQueue->pid      = GetSelfPid();
  printf("[LogCache]:Reset Cache successed\n");
  return true;
}

int Log_CacheCheck(void) {
#if defined(WIN32) || defined(LITEOS)
  return -1;
#else
  int shm_id = -1;
  void *shm_mem = ShmGet(LOG_CACHE_SHM_KEY, &shm_id);
  if (NULL == shm_mem) {
    printf("[LogCache]LogCache SHM not found, key:%d\n", LOG_CACHE_SHM_KEY);
    return -1;
  }
  // 查询Server进程是否存在
  g_pstPQueue = (IDX_QUEUE_S*)shm_mem;
  char cmd[64 + 1];
  (void)snprintf(cmd, 64, "ps aux | awk '{print $1}' | grep %d", g_pstPQueue->pid);
  int ret = system(cmd);
  if (0 != ret) {
    printf("[LogCache]Current LogCache SHM owner PID(%d) not exist\n", g_pstPQueue->pid);
    return -2;
  }
  printf("[LogCache]LogCache SHM be found, key:%d, server pid:%d\n",
         LOG_CACHE_SHM_KEY, g_pstPQueue->pid);
  return 0;
#endif
}

void Log_CacheDestory(void) {
  if (g_pstPQueue) {
    if (g_pstPQueue->iServerMode) {
      ShmClose(g_pstPQueue->iShmId, (void*)g_pstPQueue);
    } else {
      VZ_FREE(g_pstPQueue);
    }
    g_pstPQueue = NULL;
  }
}

bool Log_CacheCreate(LOG_CACHE_CFG_S *pstPQueueCfgs,
                     uint8 ucPQueueNum, bool multi_process) {
  if (g_pstPQueue) {
    return true;
  }

  do {
    IDX_QUEUE_S  *pstPQueue = NULL;
    MEM_STACK_S  *pstDStack = NULL;
    uint32        uiMemSize, uiOffset;
    uint16        usIdxBlockCnt, usDataBlockCnt, usDataBlockSize;
    uint32        i, j;

    // 入参检查
    if ((NULL == pstPQueueCfgs) || (0 == ucPQueueNum)) {
      printf("[logCache]error:invalid args!(cfg:%p,cnt:%d)\n",
             pstPQueueCfgs, ucPQueueNum);
      break;
    }

    for (i=0; i<ucPQueueNum; i++) {
      if ((0 == pstPQueueCfgs[i].usBlockNum)
          || (0 == pstPQueueCfgs[i].usBlockSize)) {
        printf("[logCache]error:invalid args!(b-cnt:%d,b-size:%d)\n",
               pstPQueueCfgs[i].usBlockNum, pstPQueueCfgs[i].usBlockSize);
        break;
      }
    }

    // 计算全部所需内存的大小，索引队列中除定制的结点外，
    // 还包括2个标记结点head和tail
    uiMemSize  = sizeof(IDX_QUEUE_S);
    uiMemSize += ucPQueueNum * sizeof(ulong);
    uiMemSize += (2 * sizeof(IDX_QUEUE_NODE_S));
    for (i=0, usIdxBlockCnt=2; i<ucPQueueNum; i++) {
      usDataBlockCnt   = pstPQueueCfgs[i].usBlockNum;
      usIdxBlockCnt   += usDataBlockCnt;

      // block size 需要按照4字节对齐修正
      usDataBlockSize  = VZ_ALIGN_ULONG(pstPQueueCfgs[i].usBlockSize);
      usDataBlockSize += MEM_STACK_DATA_PADDING;
      usDataBlockSize += sizeof(MEM_STACK_NODE_S);
      usDataBlockSize += sizeof(IDX_QUEUE_NODE_S);
      uiMemSize       += sizeof(MEM_STACK_S);
      uiMemSize       += usDataBlockCnt * usDataBlockSize;
    }

    if (multi_process) {
      int shm_id = -1;
      pstPQueue = (IDX_QUEUE_S*)ShmCreate(uiMemSize, &shm_id);
      if (NULL == pstPQueue) {
        printf("[logCache]error:malloc SHM memory failed!(size:%d)\n", uiMemSize);
        break;
      }
      pstPQueue->iShmId = shm_id;
    } else {
      pstPQueue = (IDX_QUEUE_S*)VZ_MALLOC(uiMemSize);
      if (NULL == pstPQueue) {
        printf("[logCache]error:malloc memory failed!(size:%d)\n", uiMemSize);
        break;
      }
    }
    memset(pstPQueue, 0x00, uiMemSize);

    // 初始化内存，内存布局如下:
    //
    //   IDX_QUEUE_S
    // + m*sizeof(ulong)
    // + (2+n1+n2+...)*IDX_QUEUE_NODE_S
    // + MEM_STACK_S  + n1*MEM_STACK_NODE_S + n1*(Data+Pad)
    // + MEM_STACK_S  + n2*MEM_STACK_NODE_S + n2*(Data+Pad)
    // + ...
    //
    // 注: 1，m - 内存栈的个数
    //     2，n1,n2... - 每个内存栈中结点的个数
    //     3，Data - 每个内存栈中结点的内容
    //     4，IDX_QUEUE_NODE_S是按照队列FIFO数据结构组织数据
    //     5，MEM_STACK_NODE_S是按照stack数据结构组织数据
    pstPQueue->uiMemSize       = uiMemSize;
    pstPQueue->usIdxBlockCnt   = usIdxBlockCnt;
    pstPQueue->usIdxBlockSize  = sizeof(IDX_QUEUE_NODE_S);
    pstPQueue->usHead          = 0; // head初始位置在0
    pstPQueue->ulEnQCnt        = 1; // tail初始位置在1
    pstPQueue->usMsgCnt        = 0;
    pstPQueue->ucDStackCnt     = ucPQueueNum;

    uiOffset  = ucPQueueNum * sizeof(ulong);
    uiOffset += usIdxBlockCnt * sizeof(IDX_QUEUE_NODE_S);
    for (i=0; i<ucPQueueNum; i++) {
      // 栈.头
      pstDStack = (MEM_STACK_S*)&pstPQueue->ucBody[uiOffset];
      *(((ulong*)pstPQueue->ucBody) + i) = uiOffset;
      //pstPQueue->aulOffset[i] = uiOffset;

      usDataBlockCnt   = pstPQueueCfgs[i].usBlockNum;
      pstDStack->usDataBlockCnt  = usDataBlockCnt;
      // 保证4字节对齐
      usDataBlockSize  = VZ_ALIGN_ULONG(pstPQueueCfgs[i].usBlockSize);
      usDataBlockSize += MEM_STACK_DATA_PADDING;
      pstDStack->usDataBlockSize = usDataBlockSize;
      pstDStack->ulTopAndRef     = 0;
      pstDStack->usUsedCnt       = 0;
      pstDStack->usUsedCntMax    = 0;

      uiOffset += sizeof(MEM_STACK_S);

      // 栈.索引节点 数组
      for (j = 0; j < usDataBlockCnt; j++) {
        pstDStack->astNode[j].usNext = j + 1;
      }
      pstDStack->astNode[j - 1].usNext = MEM_STACK_NODE_INVALID;

      uiOffset += sizeof(MEM_STACK_NODE_S) * usDataBlockCnt;

      // 栈.数据 数组
      for (j = 0; j < usDataBlockCnt; j++) {
        uint8 *pucData;
        ulong *pulPad;
        pucData = &pstPQueue->ucBody[uiOffset + j * usDataBlockSize];
        pulPad = (ulong*)&pucData[usDataBlockSize - MEM_STACK_DATA_PADDING];
        *pulPad = MEM_STACK_DATA_MAGIC;
      }

      uiOffset += usDataBlockSize * usDataBlockCnt;
    }

    printf("[logCache]Log cache create successed\n");
    g_pstPQueue = pstPQueue;
    g_pstPQueue->iServerMode = (int)multi_process;
    g_pstPQueue->pid = GetSelfPid();
    return true;
  } while(0);

  printf("[logCache]error:Log cache create failed\n");
  return false;
}

