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
#ifndef __LOG_CACHE_C_H__
#define __LOG_CACHE_C_H__

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


typedef void* LOG_HANDLE;

#define LOG_OK     0
#define LOG_ERROR  -1


// 队列数据dump类型
typedef enum {
  LOG_CACHE_DUMP_HEAD    = 0x01, // dump队列控制块数据
  LOG_CACHE_DUMP_IDX     = 0x02, // dump数据块索引数据
  LOG_CACHE_DUMP_OFFSET  = 0x04, // dump数据栈首地址
  LOG_CACHE_DUMP_DSTACK  = 0x08, // dump数据栈控制信息数据
  LOG_CACHE_DUMP_ALL     = 0x0F,
} LOG_CACHE_DUMP_TYPE_E;

// 队列使用信息结构体
typedef struct {
  uint16  usBlockCnt;            // 队列元素个数
  uint16  usBlockSize;           // 队列元素大小
  uint16  usBlockCntUsed;        // 队列元素当前使用个数
  uint16  usBlockCntUsedMax;     // 队列元素立历史最大使用个数
} LOG_CACHE_INFO_S;

// 内存栈配置结构体
typedef struct {
  uint16  usBlockSize;           // 内存块的大小
  uint16  usBlockNum;            // 内存块的数量
} LOG_CACHE_CFG_S;

// Log_CacheAdd回调函数，用于用户需要原子设置pvMem头信息的场景。
// pvMem 通过Log_CacheMalloc申请的内存
typedef void (*AppendDataCb) (void *pvMem);

// 调试函数，用于打印队列中的详细动态信息
// ucType 输出信息类型LOG_CACHE_DUMP_TYPE_E
// sDStackIdx 数据栈序号
void Log_CacheDump(uint8 ucType, int16 sDStackIdx);

// 将用户填写好的日志添加到索引队列
// pvMem 待插入队列数据指针
// return 成功:LOG_OK; 失败:LOG_ERROR
int Log_CacheAdd(void *pvMem);

// 从队列中提取一个数据
// return 成功:消息的地址; 失败:NULL
void* Log_CacheGet(void);

// 获取队列中日志个数
// return 成功:队列信息个数; 失败:-1
int Log_CacheCnt(void);

// 获取队列信息，可以查阅到可容纳日志个数，每个内存块大小，
// 以及当前队列中存在多少条日志，历史使用日志最大数
// pstPQueueInfo 队列信息结构
// pucQueueCnt 队列信息个数
// return 成功:队列信息个数; 失败:-1
int16 Log_CacheInfo(LOG_CACHE_INFO_S *pstPQueueInfo, uint8 *pucQueueCnt);

// 释放日志缓存
// pvMem 消息内存指针
// return 成功:0; 失败:-1:消息指针不cache范围; -2:消息指针在cache范围，但出错了
int Log_CacheFree(void *pvMem);

// 申请日志缓存
// uiSize 内存尺寸(Byte)
// return 成功:进程间通信依赖的共享内存地址; 失败:NULL
void* Log_CacheMalloc(uint32 uiSize);

// 在支持跨进程日志模式下，检查Cache是否已经创建
// return 0:cache已经存在，并且创建该缓存的进程也存在
//        -1:cache不存在
//        -2:cache存在，但是创建该缓存的进程不存在
int Log_CacheCheck(void);

// 在支持跨进程日志模式下，Reset缓存，清空缓存中的日志
// return 0:成功；-1：失败
int Log_CacheReset(void);

// 销毁日志缓存对象
void Log_CacheDestory(void);

// 创建日志缓存对象
// pstPQueueCfgs 内存池配置
// ucPQueueNum 内存栈子队列数量
// return 成功:true; 失败:false
bool Log_CacheCreate(LOG_CACHE_CFG_S *pstPQueueCfgs, uint8 ucPQueueNum, bool multi_process);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __LOG_CACHE_C_H__
