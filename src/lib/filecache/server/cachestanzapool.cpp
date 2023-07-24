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

#include "filecache/server/cachestanzapool.h"
#include "log/log/log_client.h"
#include <string.h>

namespace cache {

CachedStanzaPool *CachedStanzaPool::pool_instance_ = NULL;

CachedStanzaPool *CachedStanzaPool::Instance() {
  if (!pool_instance_) {
    pool_instance_ = new CachedStanzaPool();
  }
  return pool_instance_;
}

CachedStanzaPool::CachedStanzaPool() {
}

CachedStanzaPool::~CachedStanzaPool() {
  Queue_Destory(stanza_queue_);
}

CachedStanza::Ptr CachedStanzaPool::TakeStanza() {
  CachedStanza::Ptr cs(new CachedStanza());
  return cs;

#if 0
  CachedStanza *stanza = static_cast<CachedStanza*>(Queue_Dequeue(stanza_queue_));
  if (stanza) {
    CachedStanza::Ptr perfect_stanza;
    perfect_stanza.reset(stanza, &CachedStanzaPool::RecyleBuffer);
    //DLOG_DEBUG(MOD_EB, "Task stanza by back size %d, perfect_stanza use count %d",
    //          Queue_Size(stanza_queue_), perfect_stanza.use_count());
    return perfect_stanza;
  } else {
    DLOG_WARNING(MOD_EB, "take stanza failed, no available stanzas");
    return CachedStanza::Ptr();
  }
#endif
}

void CachedStanzaPool::RecyleStanza(CachedStanza *stanza) {
  stanza->ResetDefualtState();
  int32 ret = Queue_Enqueue(stanza_queue_, (void*)stanza);
  if (QUE_RET_OK != ret) {
    DLOG_ERROR(MOD_EB, "Recycle stanza failed, ret:%d", ret);
  }
  DLOG_DEBUG(MOD_EB, "Recycle stanza, available stanza count:%d",
             Queue_Size(stanza_queue_));
}

void CachedStanzaPool::SetDefaultCachedSize(std::size_t stanza_size) {
  DLOG_INFO(MOD_EB, "Set default stanza pool size:%d", stanza_size);
  stanza_queue_ = Queue_Create((uint32)stanza_size);
  if (NULL == stanza_queue_) {
    DLOG_ERROR(MOD_EB, "create stanza queue failed");
  } else {
    for (int i=0; i<stanza_size; i++) {
      CachedStanza *stanza = new CachedStanza();
      Queue_Enqueue(stanza_queue_, (void*)stanza);
    }
  }
}

void CachedStanzaPool::RecyleBuffer(void *stanza) {
  CachedStanzaPool::Instance()->RecyleStanza((CachedStanza *)stanza);
}

////////////////////////////////////////////////////////////////////////////////

uint32 CachedStanza::stanza_count = 0;

CachedStanza::CachedStanza()
  : is_saved_(false) {
  cache_data_ = MemBuffer::CreateMemBuffer();
  stanza_count++;
  DLOG_DEBUG(MOD_EB, "Create stanza, count:%d ", stanza_count);
}

CachedStanza::~CachedStanza() {
  stanza_count--;
  DLOG_DEBUG(MOD_EB, "Delete Stanza, count:%d", stanza_count);
}

bool CachedStanza::IsSaved() {
  vzes::CritScope stanza_mutex(&stanza_mutex_);
  return is_saved_;
}

void CachedStanza::SetData(MemBuffer::Ptr data) {
  cache_data_ = data;
}

void CachedStanza::SaveConfimation() {
  vzes::CritScope stanza_mutex(&stanza_mutex_);
  is_saved_ = true;
}

void CachedStanza::ResetDefualtState() {
  if (cache_data_) {
    cache_data_->Clear();
    cache_data_.reset();
  }

  path_.clear();
  is_saved_ = false;
}

}
