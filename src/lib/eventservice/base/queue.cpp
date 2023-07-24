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
#include <string.h>
#include "eventservice/base/common.h"
#include "eventservice/base/atomic.h"
#include "eventservice/base/queue.h"
#include "log/log/log_client.h"
#include "astl/mem_dump.h"

// 队列第index个结点地址
#define QUEUE_NODE_P(queue_entity, index) \
        ((QUEUE_NODE_S *)(queue_entity->body) + index)

// 结点准备状态
typedef enum {
  STATE_EMPTY = 0,  // 初始状态
  STATE_READY,      // 数据已经准备好
  STATE_INVALID,    // 处于无效状态
} QUEUE_STATE_E;

// 队列结点存储结构
// 该结构32位系统4字节对齐、64位系统8字节对齐
typedef struct {
  volatile ulong state;        // 节点状态 QUEUE_STATE_E
  void*          data_buffer;  // 节点数据头指针
} QUEUE_NODE_S;

// 线程间消息通信消息队列
// 该结构32位系统4字节对齐、64位系统均8字节对齐
typedef struct {
  uint32          block_cnt;   // 队列中节点个数
  uint32          block_size;  // 每个节点的尺寸
  volatile ulong  used_cnt;    // 已使用节点个数
  volatile ulong  max_use_cnt; // 历史使用的最大个数
  volatile ulong  deq_cnt;     // 出队次数版本计数器，用来计算head
  volatile ulong  enq_cnt;     // 入队次数版本计数器，用来计算tail
  uint8           body[0];     // 消息块队列数据存储区
} QUEUE_ENTITY_S;


void Queue_Dump(QUE_HANDLE hQueue, uint8 dump_type, int32 index) {
  QUEUE_ENTITY_S *queue_entity = (QUEUE_ENTITY_S *)hQueue;
  uint32 i;
  if (NULL == queue_entity) {
    return;
  }

  if (dump_type & QUEUE_DUMP_HEAD) {
    // process queue head info
    printf("Queue(%p): \r\n\tblk-cnt:%d \r\n\tblk-size:%d \r\n\tcurr-cnt:%d"
           "\r\n\tmax-use-cnt:%d \r\n\tenqcnt:%lu \r\n\tdeqcnt:%lu"
           "\r\n\thead:%d \r\n\ttail:%lu",
           queue_entity, queue_entity->block_cnt, queue_entity->block_size,
           queue_entity->used_cnt, queue_entity->max_use_cnt,
           queue_entity->enq_cnt, queue_entity->deq_cnt,
           queue_entity->deq_cnt % queue_entity->block_cnt,
           queue_entity->enq_cnt % queue_entity->block_cnt);
  }

  if (dump_type & QUEUE_DUMP_NODE) {
    QUEUE_NODE_S *pstMQueueNode;
    if (-1 == index) {
      for (i=0; i<queue_entity->block_cnt; i++) {
        pstMQueueNode = (QUEUE_NODE_S*)(queue_entity->body
                                        + i*(queue_entity->block_size));
        printf("Queue-Node%d(%p): \r\n\tstate:0x%x data:%p",
               i, pstMQueueNode, pstMQueueNode->state,
               pstMQueueNode->data_buffer);
      }
    } else {
      if ((0 <= index) && (index < queue_entity->block_cnt)) {
        pstMQueueNode = (QUEUE_NODE_S*)(queue_entity->body
                                        + index*(queue_entity->block_size));
        printf("Queue-Node%d(%p): \r\n\tstate:0x%x data:%p",
               index, pstMQueueNode, pstMQueueNode->state,
               pstMQueueNode->data_buffer);
      }
    }
  }
}

int32 Queue_Enqueue(QUE_HANDLE hQueue, void *data) {
  QUEUE_ENTITY_S *queue_entity = (QUEUE_ENTITY_S*)hQueue;
  QUEUE_NODE_S *queue_node = NULL;
  ulong old_val, new_val;
  ulong tail, next, used_cnt;

  if ((NULL == queue_entity)
      || (NULL == data)) {
    DLOG_ERROR(MOD_EB, "Enqueue failed, invalid args!(queue:%p,data:%p)",
               queue_entity, data);
    return QUE_RET_INVAL;
  }

  do {
    old_val = queue_entity->enq_cnt;
    new_val = old_val + 1;
    tail = old_val % queue_entity->block_cnt;
    if (new_val == 0) {
      // enq_cnt计数溢出(等于0)，从当前tail计算出下一个tail位置
      next = (tail + 1) % queue_entity->block_cnt;
      new_val  = next;
    } else {
      next = new_val % queue_entity->block_cnt;
    }
    // 尾追上头，队列已满。不能直接用enq_cnt和deq_cnt比较，避免
    // 计数溢出时出错
    if (next == (queue_entity->deq_cnt % queue_entity->block_cnt)) {
      //DLOG_INFO(MOD_EB, "queue is full!(queue:%p,enqcnt:%d,deqcnt:%d,"
      //          "len:%d,head:%d,tail:%d,use-cnt:%d,use-cnt-max:%d)",
      //          queue_entity, queue_entity->enq_cnt, queue_entity->deq_cnt,
      //          queue_entity->block_cnt, queue_entity->deq_cnt % queue_entity->block_cnt,
      //          tail, queue_entity->used_cnt, queue_entity->max_use_cnt);
      return QUE_RET_QFULL;
    }
  } while (!VZ_BCAS(&(queue_entity->enq_cnt), old_val, new_val));

  // 填充结点
  queue_node = QUEUE_NODE_P(queue_entity, tail);
  queue_node->data_buffer = data;
  queue_node->state = STATE_READY;

  // 记录当前队列中节点使用个数，以及历史节点最大使用个数
  used_cnt = VZ_AAF(&(queue_entity->used_cnt), 1);
  if (used_cnt > queue_entity->max_use_cnt) {
    (void)VZ_LTAS(&(queue_entity->max_use_cnt), used_cnt);
  }
  return QUE_RET_OK;
}

void* Queue_Dequeue(QUE_HANDLE hQueue) {
  QUEUE_ENTITY_S *queue_entity = (QUEUE_ENTITY_S*)hQueue;
  QUEUE_NODE_S *queue_node = NULL;
  ulong old_val, new_val;
  ulong head, next;

  if (NULL == queue_entity) {
    DLOG_ERROR(MOD_EB, "Invalid queue handle");
    return NULL;
  }

  do {
    old_val = queue_entity->deq_cnt;
    new_val = old_val + 1;
    if (new_val == 0) {
      // deq_cnt计数溢出(等于0)，从当前head计算出下一个head位置
      head = old_val % queue_entity->block_cnt;
      next = (head + 1) % queue_entity->block_cnt;
      new_val  = next;
    } else {
      next = new_val % queue_entity->block_cnt;
    }
    // 头追上尾，队列为空。不能直接用enq_cnt和deq_cnt比较，避免
    // 计数溢出时出错
    if (next == (queue_entity->enq_cnt % queue_entity->block_cnt)) {
      return NULL;
    }
    queue_node = QUEUE_NODE_P(queue_entity, next);
    if (STATE_READY != VZ_FAA(&(queue_node->state), 0)) {
      // 头结点未写入完成
      return NULL;
    }
  } while (!VZ_BCAS(&(queue_entity->deq_cnt), old_val, new_val));

  //queue_node = QUEUE_NODE_P(queue_entity, next);
  void *node_buffer = queue_node->data_buffer;
  queue_node->state = STATE_EMPTY;
  queue_node->data_buffer = NULL;
  (void)VZ_FAS(&(queue_entity->used_cnt), 1);
  return node_buffer;
}

int32 Queue_Size(QUE_HANDLE hQueue) {
  QUEUE_ENTITY_S *queue_entity = (QUEUE_ENTITY_S*)hQueue;
  if (NULL == queue_entity) {
    DLOG_ERROR(MOD_EB, "Invalid queue handle");
    return QUE_RET_INVAL;
  }

  return queue_entity->used_cnt;
}

int32 Queue_GetInfo(QUE_HANDLE hQueue, QUEUE_INFO_S *queue_info) {
  QUEUE_ENTITY_S *queue_entity = (QUEUE_ENTITY_S*)hQueue;
  if ((NULL == queue_entity)
      || (NULL == queue_info)) {
    DLOG_ERROR(MOD_EB, "Invalid args!(queue:%p,info:%p)",
               queue_entity, queue_info);
    return QUE_RET_INVAL;
  }
  queue_info->queue_length  = queue_entity->block_cnt - 2;
  queue_info->curr_used_num = queue_entity->used_cnt;
  queue_info->max_used_num  = queue_entity->max_use_cnt;
  return QUE_RET_OK;
}

void Queue_Destory(QUE_HANDLE hQueue) {
  if (NULL == hQueue) {
    DLOG_ERROR(MOD_EB, "Invalid queue handle!");
    return;
  }
  VZ_FREE(hQueue);
}

QUE_HANDLE Queue_Create(uint32 queue_length) {
  QUEUE_ENTITY_S *queue_entity = NULL;
  uint32 size;
  if (0 == queue_length) {
    DLOG_ERROR(MOD_EB, "Invalid queue length %d", queue_length);
    return NULL;
  }

  // 额外申请2个节点，用于标记队列的head和tail节点,
  // head初始位置在0, tail初始位置在1,
  // 从tail当前位置插入数据，从head+1位置取数据，
  // 当(head+1) == tail时，队列为空；(tail+1) == head时，队列已满
  size = sizeof(QUEUE_ENTITY_S) + (queue_length + 2) * sizeof(QUEUE_NODE_S);
  queue_entity = (QUEUE_ENTITY_S*)VZ_MALLOC(size);
  if (NULL == queue_entity) {
    DLOG_ERROR(MOD_EB, "Malloc memory failed!(len:%d,size:%d)",
               queue_length, size);
    return NULL;
  }
  memset(queue_entity, 0x00, size);
  queue_entity->block_cnt   = queue_length + 2;
  queue_entity->block_size  = sizeof(QUEUE_NODE_S);
  queue_entity->deq_cnt     = 0; // head初始位置在0
  queue_entity->enq_cnt     = 1; // tail初始位置在1
  queue_entity->used_cnt    = 0;
  queue_entity->max_use_cnt = 0;
  return ((QUE_HANDLE)queue_entity);
}
