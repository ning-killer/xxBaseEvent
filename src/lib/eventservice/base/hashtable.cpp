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
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include "eventservice/base/hashtable.h"
#include "log/log/log_client.h"
#include "astl/mem_dump.h"

namespace vzes {

// 哈希表节点信息结构体
typedef struct hash_node {
  void                 *key;        // 哈希表节点key指针
  void                 *data;       // 哈希表节点用户数据指针
  struct hash_node     *next;       // 哈希桶中下一个节点指针
} hashtable_node_s;


// 哈希表信息结构体
typedef struct {
  hashtable_node_s    **hash_table; // 哈希表指针
  uint32                slot_num;   // 哈希表中槽位的数量
  uint32                node_num;   // 哈希表中当前节点的数量, 最大值为 HASHTAB_MAX_NODES
  hashtable_usr_func_s  usr_func;   // 用户自定义的回调函数
} hashtable_entity_s;


uint32 Hash_AlgTW(uint32 key) {
  key = ~key + (key << 15); // key = (key << 15) - key - 1;
  key = key ^ (key >> 12);
  key = key + (key << 2);
  key = key ^ (key >> 4);
  key = key * 2057;           // key = (key + (key << 3)) + (key << 11);
  key = key ^ (key >> 16);
  return key;
}

uint32 Hash_AlgBJ(uint32 key) {
  key = (key + 0x7ed55d16) + (key << 12);
  key = (key ^ 0xc761c23c) ^ (key >> 19);
  key = (key + 0x165667b1) + (key << 5);
  key = (key + 0xd3a2646c) ^ (key << 9);
  key = (key + 0xfd7046c5) + (key << 3); // <<和 +的组合是可逆的
  key = (key ^ 0xb55a4f09) ^ (key >> 16);
  return key;
}

uint32 Hash_AlgRS(char *key, uint32 len) {
  uint32 hash_val = 0;
  uint32 factor_a = 63689;
  uint32 factor_b = 378551;
  uint32 i;

  for (i = 0; i < len; key++, i++) {
    hash_val = hash_val * factor_a + (*key);
    factor_a = factor_a * factor_b;
  }

  return hash_val;
}

uint32 Hash_AlgJS(char *key, uint32 len) {
  uint32 hash_val = 1315423911;
  uint32 i;

  for (i = 0; i < len; key++, i++) {
    hash_val ^= ((hash_val << 5) + (*key) + (hash_val >> 2));
  }

  return hash_val;
}

uint32 Hash_AlgBKDR(char *key, uint32 len) {
  uint32 seed     = 131; // 31 131 1313 13131 131313 etc..
  uint32 hash_val = 0;
  uint32 i;

  for (i = 0; i < len; key++, i++) {
    hash_val = (hash_val * seed) + (*key);
  }

  return hash_val;
}

uint32 Hash_AlgSDBM(char *key, uint32 len) {
  uint32 hash_val = 0;
  uint32 i;

  for (i = 0; i < len; key++, i++) {
    hash_val = (*key) + (hash_val << 6) + (hash_val << 16) - hash_val;
  }

  return hash_val;
}

HASH_HANDLE Hash_TabCreate(uint32 uiSize,
                           hashtable_usr_func_s *usr_func) {
  hashtable_entity_s *hash_table = NULL;
  uint32              len;
  uint32              i;

  do {
    if ((0 == uiSize)
        || (NULL == usr_func)
        || (NULL == usr_func->pfnHashValue)
        || (NULL == usr_func->pfnKeyCmp)) {
      DLOG_ERROR(MOD_EB, "Create hash table failed, invalid args");
      break;
    }

    len = sizeof(*hash_table);
    hash_table = (hashtable_entity_s*)VZ_MALLOC(len);
    if (NULL == hash_table) {
      DLOG_ERROR(MOD_EB, "alloc mem failed, size = %d", len);
      break;
    }

    memset(hash_table, 0x00, len);
    hash_table->slot_num = uiSize;
    memcpy(&(hash_table->usr_func), usr_func, sizeof(*usr_func));
    len = sizeof(*(hash_table->hash_table)) * uiSize;
    hash_table->hash_table = (hashtable_node_s**)VZ_MALLOC(len);
    if (NULL == hash_table->hash_table) {
      DLOG_ERROR(MOD_EB, "alloc mem failed, memSize = %d, soltSize = %d",
                 len, uiSize);
      VZ_FREE(hash_table);
      break;
    }

    for (i = 0; i < uiSize; i++) {
      hash_table->hash_table[i] = NULL;
    }

    DLOG_INFO(MOD_EB, "Hash table create successed, size = %d", uiSize);
    return (HASH_HANDLE)hash_table;
  } while(0);

  DLOG_ERROR(MOD_EB, "create hash table failed");
  return NULL;
}

void Hash_TabDestroy(HASH_HANDLE hHashtab) {
  hashtable_entity_s *hash_table   = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *current_node = NULL;
  hashtable_node_s   *temp_node    = NULL;
  uint32              i;

  if (NULL == hash_table) {
    DLOG_ERROR(MOD_EB, "destory Hashtable failed, invalid args");
    return;
  }

  for (i = 0; i < hash_table->slot_num; i++) {
    current_node = hash_table->hash_table[i];
    while (current_node) {
      temp_node = current_node;
      current_node = current_node->next;
      if (hash_table->usr_func.pfnFree) {
        hash_table->usr_func.pfnFree(temp_node->data);
      }
      VZ_FREE(temp_node);
    }
    hash_table->hash_table[i] = NULL;
  }

  VZ_FREE(hash_table->hash_table);
  hash_table->hash_table = NULL;
  VZ_FREE(hash_table);
}

int32 Hash_TabInsert(HASH_HANDLE hHashtab, void *key, void *data) {
  hashtable_entity_s *hash_table   = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *prev_node    = NULL;
  hashtable_node_s   *current_node = NULL;
  hashtable_node_s   *new_node     = NULL;
  uint32              value;

  if ((NULL == hash_table)
      || (NULL == key)) {
    DLOG_ERROR(MOD_EB, "insert element failed, invalid args, hHash:%p, key:%p",
               hash_table, key);
    return -1;
  }

  if (HASHTAB_MAX_NODES <= hash_table->node_num) {
    DLOG_ERROR(MOD_EB, "insert element failed, reach max node num, nodeNum: %d",
               hash_table->node_num);
    return -1;
  }

  value = hash_table->usr_func.pfnHashValue(key);
  value = value % hash_table->slot_num;
  current_node = hash_table->hash_table[value];
  while (current_node
         && hash_table->usr_func.pfnKeyCmp(key, current_node->key) > 0) {
    prev_node = current_node;
    current_node  = current_node->next;
  }

  if (current_node
      && (hash_table->usr_func.pfnKeyCmp(key, current_node->key) == 0)) {
    DLOG_ERROR(MOD_EB, "insert element failed, already exist");
    return -1;
  }

  new_node = (hashtable_node_s*)VZ_MALLOC(sizeof(*new_node));
  if (NULL == new_node) {
    DLOG_ERROR(MOD_EB, "insert element failed, alloc mem failed,Size: %d",
               sizeof(*new_node));
    return -1;
  }

  new_node->key  = key;
  new_node->data = data;
  new_node->next = NULL;
  if (prev_node) {
    new_node->next = prev_node->next;
    prev_node->next  = new_node;
  } else {
    new_node->next = hash_table->hash_table[value];
    hash_table->hash_table[value] = new_node;
  }

  hash_table->node_num ++;
  return 0;
}

int32 Hash_TabDelete(HASH_HANDLE hHashtab, const void *key) {
  hashtable_entity_s *hash_table   = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *current_node = NULL;
  hashtable_node_s   *prev_node    = NULL;
  uint32              value;

  if ((NULL == hash_table)
      || (NULL == key)) {
    DLOG_ERROR(MOD_EB, "delete element failed, invalid args");
    return -1;
  }

  value = hash_table->usr_func.pfnHashValue(key);
  value = value % hash_table->slot_num;
  current_node = hash_table->hash_table[value];
  while (current_node
         && hash_table->usr_func.pfnKeyCmp(key, current_node->key) > 0) {
    prev_node = current_node;
    current_node = current_node->next;
  }

  if (current_node
      && (hash_table->usr_func.pfnKeyCmp(key, current_node->key) == 0)) {
    if (NULL == prev_node) {
      hash_table->hash_table[value] = current_node->next;
    } else {
      prev_node->next = current_node->next;
    }

    if (hash_table->usr_func.pfnFree) {
      hash_table->usr_func.pfnFree(current_node->data);
    }

    hash_table->node_num --;
    VZ_FREE(current_node);
    return 0;
  }

  DLOG_ERROR(MOD_EB, "delete hash table element failed, not found");
  return -1;
}

void* Hash_TabSearch(HASH_HANDLE hHashtab, const void *key) {
  hashtable_entity_s *hash_table   = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *current_node = NULL;
  uint32              value;

  if ((NULL == hash_table)
      || (NULL == key)) {
    DLOG_ERROR(MOD_EB, "search element failed, invalid args");
    return NULL;
  }

  value = hash_table->usr_func.pfnHashValue(key);
  value = value % hash_table->slot_num;
  current_node = hash_table->hash_table[value];
  while (current_node
         && hash_table->usr_func.pfnKeyCmp(key, current_node->key) > 0) {
    current_node = current_node->next;
  }

  if ((current_node == NULL)
      || (hash_table->usr_func.pfnKeyCmp(key, current_node->key) != 0)) {
    return NULL;
  }

  return current_node->data;
}

void Hash_TabStat(HASH_HANDLE hHashtab, hashtable_info_s *info) {
  hashtable_entity_s *hash_table    = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *current_node  = NULL;
  uint32              chain_len     = 0;
  uint32              max_chain_len = 0;
  uint32              slots_used    = 0;
  uint32              i;

  if ((NULL == hash_table)
      || (NULL == info)) {
    DLOG_ERROR(MOD_EB, "Stat Hash Table failed, invalid args");
    return;
  }

  for (i = 0; i < hash_table->slot_num; i++) {
    current_node = hash_table->hash_table[i];
    if (current_node) {
      slots_used ++;
      chain_len = 0;
      while (current_node) {
        chain_len ++;
        current_node = current_node->next;
      }

      if (chain_len > max_chain_len) {
        max_chain_len = chain_len;
      }
    }
  }

  info->slot_used      = slots_used;
  info->node_num       = hash_table->node_num;
  info->max_bucket_len = max_chain_len;
}

void Hash_TabDump(HASH_HANDLE hHashtab) {
  hashtable_entity_s *hash_table   = (hashtable_entity_s*)hHashtab;
  hashtable_node_s   *current_node = NULL;
  uint32              i;

  if (NULL == hash_table) {
    return;
  }

  for (i = 0; i < hash_table->slot_num; i++) {
    current_node = hash_table->hash_table[i];
    while (current_node) {
      if (hash_table->usr_func.pfnDump) {
        hash_table->usr_func.pfnDump(current_node->key, current_node->data);
      }

      current_node = current_node->next;
    }
  }

  return;
}

}// namespace vzes
