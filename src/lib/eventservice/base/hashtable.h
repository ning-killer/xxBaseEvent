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
*
* @file hashtable.h
* @defgroup hashtable
* @{ @ingroup Vzenith
*
* @brief 移植Linux内核Hash table实现
* 使用步骤:
* 1) 定义哈希表回调函数结构体HASHTAB_USR_FUNC_S。其中，必须定义pfnHashValue和pfnKeyCmp，
*    如果销毁哈希表时需要释放哈希data，或者想dump哈希表所有节点信息，也需要定义pfnFree和pfnDump。
* 2) 调用frwk_HashtabCreate创建哈希表。
* 3) 哈希表创建成功后，可以调用Hash_TabInsert、Hash_TabDelete、Hash_TabSearch、
*    Hash_TabStat、Hash_TabDump分别进行插入哈希节点、删除哈希节点、查询哈希节点、
*    统计哈希表使用信息、dump哈希表所有节点等操作。
* 4) 哈希表不再使用时，调用Hash_TabDestroy销毁哈希表。其中，如果用户定义了pfnFree函数，会调用
*    该函数释放用户数据。
*/

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


/******************************************************************************
 *                       Typedef/Enum/Struct/Union
 ******************************************************************************/
namespace vzes {

#define HASHTAB_MAX_NODES	0xffffffff  // 哈希表中节点的最大数量 

typedef void* HASH_HANDLE;


// 用户自定义接口
typedef struct {
  // 可选: 用于释放用户数据
  void  (*pfnFree) (void *data);

  // 可选: 用于Dump哈希表中节点(key, value)信息
  void (*pfnDump)(void *key, void *data);

  // 必选: 用于计算哈希值, key可以是int、long、char*任何类型。
  // 用户可自定义哈希函数，也可以使用平台提供的哈希函数。
  // 哈希函数的选择要尽量保证哈希值的均匀分布，若key值为整数，最高效的方式
  // 就是选择&位运算的算法；若为字符串型则有多种选择如：RS、JS、BKDR、SDBM等。
  // 注意: 返回的哈希值在FRWK内部会映射到哈希表的Size范围内，该函数中不需要转换。
  uint32 (*pfnHashValue)(const void *key);

  // 必选: 用于比较哈希key, key可以是int、long、char*任何类型。
  // 注意: 如果两个key不相等，须返回大于0。
  //  *key1 != *key2, 返回 > 0
  //  *key1 == *key2, 返回 0
  int32 (*pfnKeyCmp)(const void *key1, const void *key2);
} hashtable_usr_func_s;


// 哈希表统计信息结构体
typedef struct {
  uint32   slot_used;       // 槽位使用数量
  uint32   node_num;        // 节点(元素)的数量
  uint32   max_bucket_len;  // 哈希桶最大深度
} hashtable_info_s;


/******************************************************************************
 *                         Application Interfaces
 ******************************************************************************/

// 整数Hash算法, Tomas Wang哈希
// key,哈希Key
// return,哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgTW(uint32 key);

// 整数Hash算法, Bob Jenkins哈希
// key, 哈希Key
// return, 哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgBJ(uint32 key);

// 字符串Hash算法, RS哈希
// key, 哈希Key字符串
// len, Key的长度
// return, 哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgRS(char *key, uint32 len);

// 字符串Hash算法, JS哈希
// key, 哈希Key字符串
// len, Key的长度
// return, 哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgJS(char *key, uint32 len);

// 字符串Hash算法, BKDR哈希
// key, 哈希Key字符串
// len, Key的长度
// return, 哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgBKDR(char *key, uint32 len);

// 字符串Hash算法, SDBM哈希
// key, 哈希Key字符串
// len, Key的长度
// return, 哈希值，取值范围0 ~ 0xFFFFFFFF
uint32 Hash_AlgSDBM(char *key, uint32 len);

// 创建一个新的哈希表(hashtable)。!!! hash表可以用于大规模数据下的增加、删除操作；
// 但是若存在一些遍历的需求，hash表在这块的效率不高（需要遍历所有的桶），这些情况
// 则可以考虑别的数据结构如红黑树、B+树等。
// size, 哈希表中槽位的数量，!!!注意:取值最好为素数，这样取余操作时会产生最分散的余数；
// usr_func, 用户自定义回调函数参数
// return,成功，哈希表句柄; 失败，FRWK_NULL
HASH_HANDLE Hash_TabCreate(uint32 size, hashtable_usr_func_s *usr_func);

// 销毁哈希表
// hHashtab, 哈希表句柄
void Hash_TabDestroy(HASH_HANDLE hHashtab);

// 插入指定的(key, data)到哈希表中
// hHashtab, 哈希表句柄
// key, key指针，可以是int、long、char*任何类型
// data, key对应的用户数据指针
// return,成功，0; 失败，< 0.
int32 Hash_TabInsert(HASH_HANDLE hHashtab, void *key, void *data);

// 删除指定的key对应的hash节点
// hHashtab, 哈希表句柄
// key, key指针，可以是int、long、char*任何类型
// return, 成功，该key对应的用户数据指针;失败，FRWK_NULL
int32 Hash_TabDelete(HASH_HANDLE hHashtab, const void *key);

// 查找指定的key对应的用户数据
// hHashtab, 哈希表句柄
// key, key指针，可以是int、long、char*任何类型
// return, 成功，该key对应的用户数据指针;失败，FRWK_NULL
void* Hash_TabSearch(HASH_HANDLE hHashtab, const void *key);

// 统计当前哈希表使用信息
// hHashtab, 哈希表句柄
// param info, 输出参数，当前哈希表统计信息
void Hash_TabStat(HASH_HANDLE hHashtab, hashtable_info_s *info);

// 调试接口，用于Dump哈希表中节点信息(key, value)
// hHashtab, 哈希表句柄
void Hash_TabDump(HASH_HANDLE hHashtab);

}// namespace vzes

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _HASHTABLE_H_ */

