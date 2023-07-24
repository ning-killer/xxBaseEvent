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

#ifndef __LOG_LF_QUEUE_C_H__
#define __LOG_LF_QUEUE_C_H__

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


typedef void* QUE_HANDLE;

#define QUE_RET_OK       0  // 成功
#define QUE_RET_ERROR   -1  // 失败
#define QUE_RET_QFULL   -2  // 队列已满
#define QUE_RET_INVAL   -3  // 无效的参数

// 队列数据dump类型
typedef enum {
  QUEUE_DUMP_HEAD  = 0x01,  // dump队列控制块数据
  QUEUE_DUMP_NODE  = 0x10,  // dump队列数据块信息头数据
  QUEUE_DUMP_ALL   = 0x11,
} QUEUE_DUMP_TYPE_E;

// 队列使用信息结构体
typedef struct {
  uint32  queue_length;     // 队列节点个数
  uint32  curr_used_num;    // 队列节点当前使用个数
  uint32  max_used_num;     // 队列节点历史最大使用个数
} QUEUE_INFO_S;


// 创建FIFO队列实例，入队\出队支持多线程并发
// queue_length:队列长度，不支持动态扩展，
// 每个节点内存开销: 8字节(32位系统)；16字节(64位系统)
// return 成功：对象句柄；失败：NULL
QUE_HANDLE Queue_Create(uint32 queue_length);

// 销毁队列
void Queue_Destory(QUE_HANDLE hQueue);

// 出队
// return 成功：用户数据指针；失败：NULL
void* Queue_Dequeue(QUE_HANDLE hQueue);

// 入队
// data：用户数据buffer指针
// return 成功：QUE_RET_OK；失败：< 0
int32 Queue_Enqueue(QUE_HANDLE hQueue, void *data);

// 获取当前队列中数据数量
// return 成功：>= 0；失败：< 0
int32 Queue_Size(QUE_HANDLE hQueue);

// 获取队列信息
// queue_info：信息输出结构buffer
// return 成功：QUE_RET_OK；失败：< 0
int32 Queue_GetInfo(QUE_HANDLE hQueue, QUEUE_INFO_S *queue_info);

// 调试接口，打印当前队列详细信息
// dump_type：信息的类型 QUEUE_DUMP_TYPE_E
// index：节点序号。[0, queue_length-1]：指定单个节点；-1：所有的节点
void Queue_Dump(QUE_HANDLE hQueue, uint8 dump_type, int32 index);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __LOG_LF_QUEUE_C_H__
