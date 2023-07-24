/*
* vzsdk
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

#include "eventservice/mem/membuffer.h"
#include "eventservice/base/queue.h"
#include "eventservice/base/basictypes.h"
#include <string.h>
#include <stdio.h>

namespace vzes {

unsigned int g_block_count = 0;
unsigned int g_membuffer_count = 0;
////////////////////////////////////////////////////////////////////////////////

class BlockManager : public boost::noncopyable {
 public:
  static BlockManager *Instance();
  Block::Ptr TakeBlock() {
    Block::Ptr block(new Block());
    return block;

#if 0
    Block *block = static_cast<Block*>(Queue_Dequeue(blocks_queue_));
    if (NULL == block) {
      block = new Block();
      blocks_total_ ++;
    }
    return Block::Ptr(block, BlockManager::RecyleBlock);
#endif
  }
  static void RecyleBlock(void *block);
 public:
  void InternalRecyleBlock(Block *block) {
    block->buffer[0]    = 0;
    block->buffer_size  = 0;
    block->encode_flag_ = false;
    int ret = Queue_Enqueue(blocks_queue_, (void*)block);
    if (QUE_RET_QFULL == ret) {
      delete block;
      blocks_total_ --;
    }
  }
  BlockManager() {
    blocks_total_ = 0;
    blocks_queue_ = Queue_Create(blocks_capacity_);
    if (NULL == blocks_queue_) {
      DLOG_ERROR(MOD_EB, "block manager create queue failed");
    }
  }
  virtual ~BlockManager() {
  }

  void DumpBlocksInfo() {
    printf("> blocks %u membuffer %u.\n",
           g_block_count, g_membuffer_count);

    //QUEUE_INFO_S queue_info;
    //(void)Queue_GetInfo(blocks_queue_, &queue_info);
    //printf("> membuffer blocks queue info:"
    //       "len %d, use_cnt %d, max_use_cnt %d\n",
    //       queue_info.queue_length,
    //       queue_info.curr_used_num,
    //       queue_info.max_used_num);
  }

 private:
  static const uint32   blocks_capacity_ = 1024;
  uint32                blocks_total_;
  QUE_HANDLE            blocks_queue_;
  static BlockManager  *instance_;
};

BlockManager *BlockManager::instance_ = NULL;
BlockManager *BlockManager::Instance() {
  if (instance_ == NULL) {
    instance_ = new BlockManager();
  }
  return instance_;
}

void BlockManager::RecyleBlock(void *block) {
  if (block != NULL) {
    BlockManager::Instance()->InternalRecyleBlock((Block *)block);
  } else {
    DLOG_ERROR(MOD_EB, "Block is null");
  }
}

////////////////////////////////////////////////////////////////////////////////
MemBuffer::Ptr MemBuffer::CreateMemBuffer() {
  return MemBuffer::Ptr(new MemBuffer());
}

MemBuffer::MemBuffer() : size_(0) {
  g_membuffer_count++;
}

MemBuffer::~MemBuffer() {
  g_membuffer_count--;
}

size_t MemBuffer::Length() const {
  return size_;
}

size_t MemBuffer::size() const {
  return size_;
}

// Read a next value from the buffer. Return false if there isn't
// enough data left for the specified type.
bool MemBuffer::ReadUInt8(uint8* val) {
  return ReadBytes((char *)val, sizeof(uint8));
}

bool MemBuffer::ReadUInt16(uint16* val) {
  return ReadBytes((char *)val, sizeof(uint16));
}

bool MemBuffer::ReadUInt32(uint32* val) {
  return ReadBytes((char *)val, sizeof(uint32));
}

bool MemBuffer::ReadUInt64(uint64* val) {
  return ReadBytes((char *)val, sizeof(uint64));
}

bool MemBuffer::ReadBytes(char* val, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = 0;
    if (val == NULL) {
      rs = (*iter)->ReadBytes(NULL, len - read_size);
    } else {
      rs = (*iter)->ReadBytes(val + read_size, len - read_size);
    }
    read_size += rs;
    if ((*iter)->buffer_size == 0) {
      iter = blocks_.erase(iter) ;
    } else {
      iter ++;
    }
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}

bool MemBuffer::ReadBuffer(MemBuffer::Ptr buffer, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = 0;
    if (len - read_size >= (*iter)->buffer_size) {
      rs = (*iter)->buffer_size;
      buffer->WriteBytes((const char *)((*iter)->buffer), rs);
      iter = blocks_.erase(iter);
    } else {
      rs = len - read_size;
      buffer->WriteBytes((const char *)((*iter)->buffer), rs);
      (*iter)->ReadBytes(NULL, rs);
      iter++;
    }
    read_size += rs;
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}

// Appends next |len| bytes from the buffer to |val|. Returns false
// if there is less than |len| bytes left.
bool MemBuffer::ReadString(vzstd::string* val, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = (*iter)->ReadString(val, len - read_size);
    read_size += rs;
    if ((*iter)->buffer_size == 0) {
      iter = blocks_.erase(iter) ;
    } else {
      iter ++;
    }
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}


vzstd::string MemBuffer::ToString() {
  vzstd::string result;
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    result.append((const char *)((*iter)->buffer), (*iter)->buffer_size);
  }
  return result;
}


void MemBuffer::DumpData() {
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    LOG(L_INFO).write((const char *)((*iter)->buffer), (*iter)->buffer_size);
  }
}

// -----------------------------------------------------------------------------

void MemBuffer::AppendBuffer(MemBuffer::Ptr buffer) {
  BlocksPtr &blocks = buffer->blocks();
  blocks_.insert(blocks_.end(), blocks.begin(), blocks.end());
  size_ = size_ + buffer->size();
}

void MemBuffer::AppendBlock(Block::Ptr block) {
  blocks_.push_back(block);
  size_ = size_ + block->buffer_size;
}

void MemBuffer::Clear() {
  blocks_.clear();
  size_ = 0;
}

bool MemBuffer::EnableEncode(bool enable) {
  if (blocks_.size() == 0) {
    return false;
  }
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    Block::Ptr block = *iter;
    block->encode_flag_ = enable;
  }
  return true;
}

// Copy a next value from the buffer. Return false if there isn't
// enough data left for the specified type.
bool MemBuffer::CopyUInt8(size_t pos, uint8* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint8));
}

bool MemBuffer::CopyUInt16(size_t pos, uint16* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint16));
}

bool MemBuffer::CopyUInt32(size_t pos, uint32* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint32));
}

bool MemBuffer::CopyUInt64(size_t pos, uint64* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint64));
}

bool MemBuffer::CopyBytes(size_t pos, char* val, size_t len) {
  if (size_ - pos < len) {
    return false;
  }

  (void)CopyBytes(val, pos, len);
  return true;
}

bool MemBuffer::CopyBuffer(MemBuffer::Ptr buffer, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = VZ_MIN(len - read_size, (*iter)->buffer_size);
    buffer->WriteBytes((const char *)((*iter)->buffer), rs);
    read_size += rs;
    if (read_size == len) {
      break;
    }
    iter++;
  }
  return true;
}

size_t MemBuffer::CopyBytes(char* val, size_t pos, size_t len) {
  if (pos >= size_) {
    return 0;
  }
  size_t act_len = ((pos + len) > size_) ? (size_ - pos) : (len);
  size_t block_pos = 0;
  BlocksPtr::iterator iter = GetPostion(pos, &block_pos);
  size_t total_cs = 0;
  while(iter != blocks_.end()) {
    size_t cs = (*iter)->CopyBytes(block_pos, val + total_cs, act_len - total_cs);
    total_cs += cs;
    if (total_cs == act_len) {
      break;
    }
    block_pos = 0;
    iter++;
  }
  return act_len;
}

// Appends next |len| bytes from the buffer to |val|. Returns false
// if there is less than |len| bytes left.
bool MemBuffer::CopyString(size_t pos, vzstd::string* val, size_t len) {
  if (size_ - pos < len) {
    return false;
  }
  size_t block_pos = 0;
  BlocksPtr::iterator iter = GetPostion(pos, &block_pos);
  size_t total_cs = 0;
  while(iter != blocks_.end()) {
    size_t cs = (*iter)->CopyString(block_pos, val, len - total_cs);
    total_cs += cs;
    if (total_cs == len) {
      break;
    }
    block_pos = 0;
    iter++;
  }
  return true;
}

BlocksPtr::iterator MemBuffer::GetPostion(size_t pos, size_t *block_pos) {
  if (pos >= size_) {
    return blocks_.end();
  }
  size_t remain_pos = pos;
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    int rs = (*iter)->buffer_size - remain_pos;
    if (rs > 0) {
      *block_pos = remain_pos;
      return iter;
    } else {
      remain_pos = remain_pos - (*iter)->buffer_size;
    }
  }
  return blocks_.end();
}

// -----------------------------------------------------------------------------
// Write value to the buffer. Resizes the buffer when it is
// neccessary.
void MemBuffer::WriteUInt8(uint8 val) {
  WriteBytes((const char *)&val, sizeof(uint8));
}

void MemBuffer::WriteUInt16(uint16 val) {
  WriteBytes((const char *)&val, sizeof(uint16));
}

void MemBuffer::WriteUInt32(uint32 val) {
  WriteBytes((const char *)&val, sizeof(uint32));
}

void MemBuffer::WriteUInt64(uint64 val) {
  WriteBytes((const char *)&val, sizeof(uint64));
}

void MemBuffer::WriteString(const vzstd::string& val) {
  WriteBytes(val.c_str(), val.size());
}

void MemBuffer::WriteBytes(const char* val, size_t len) {
  if (blocks_.size() == 0) {
    WriteNewBytes(val, len);
  } else {
    // 1. 先检查最后一个Buffer是否有可以写的空间
    Block::Ptr last_block = blocks_.back();
    size_t ws = 0;
    if (last_block->RemainSize() != 0) {
      ws = last_block->WriteBytes(val, len);
    }
    // 2. 如果数据没有写完，就直接写新的Block
    if (len > ws) {
      WriteNewBytes(val + ws, len - ws);
    }
  }
  size_ = size_ + len;
}

void MemBuffer::WriteNewBytes(const char* val, size_t len) {
  size_t pos = 0;
  while(true) {
    Block::Ptr block = BlockManager::Instance()->TakeBlock();
    blocks_.push_back(block);
    size_t ws = block->WriteBytes(val + pos, len - pos);
    pos += ws;
    if (pos == len) {
      return ;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////


Block::Ptr Block::TakeBlock() {
  return BlockManager::Instance()->TakeBlock();
}

void Block::DumpBlocksInfo() {
  BlockManager::Instance()->DumpBlocksInfo();
}

size_t Block::WriteBytes(const char* val, size_t len) {
  if (DEFAULT_BLOCK_SIZE == buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = DEFAULT_BLOCK_SIZE - buffer_size;
  size_t write_size  = remain_size > len ? len : remain_size;
  memcpy(buffer + buffer_size, val, write_size);
  buffer_size += write_size;
  return write_size;
}

size_t Block::ReadBytes(char* val, size_t len) {
  if (0 == buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size;
  size_t read_size   = remain_size > len ? len : remain_size;
  if (val != NULL) {
    memcpy(val, buffer, read_size);
  }
  remain_size = buffer_size - read_size;
  memmove(buffer, buffer + read_size, remain_size);
  buffer_size = remain_size;
  return read_size;
}


size_t Block::ReadString(vzstd::string* val, size_t len) {
  if (0 == buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size;
  size_t read_size   = remain_size > len ? len : remain_size;
  val->append((const char *)buffer, read_size);
  remain_size = buffer_size - read_size;
  memmove(buffer, buffer + read_size, remain_size);
  buffer_size = remain_size;
  return read_size;
}


size_t Block::CopyBytes(size_t pos, char* val, size_t len) {
  if (pos > buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size - pos;
  size_t copy_size   = remain_size > len ? len : remain_size;
  memcpy(val, buffer + pos, copy_size);
  return copy_size;
}

size_t Block::CopyString(size_t pos, vzstd::string* val, size_t len) {
  if (pos > buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size - pos;
  size_t copy_size   = remain_size > len ? len : remain_size;
  val->append((const char *)(buffer + pos), copy_size);
  return copy_size;
}

}


