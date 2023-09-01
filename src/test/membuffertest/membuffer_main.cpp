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

#include <iostream>
#include "eventservice/mem/membuffer.h"
#include <string.h>
#include <stdio.h>

#define TEST_DATA_SIZE 1024
const char TEST_DATA[TEST_DATA_SIZE] = {0};

void MemBufferReadWriteBigTest() {
  DLOG_INFO(MOD_EB, "--------------------------------------------------------");
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  DLOG_INFO(MOD_EB, "MemBufferReadWriteBigTest Write start,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  mb->WriteBytes(TEST_DATA, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == TEST_DATA_SIZE);
  DLOG_INFO(MOD_EB, "MemBufferReadWriteBigTest Write End,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  mb->CopyBytes(0, read_buffer, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == TEST_DATA_SIZE);
  DLOG_INFO(MOD_EB, "MemBufferReadWriteBigTest Copy End,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  mb->ReadBytes(read_buffer, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == 0);
  DLOG_INFO(MOD_EB, "MemBufferReadWriteBigTest Read End,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
}

void MemBufferReadWriteCorrectTest() {
  DLOG_INFO(MOD_EB, "--------------------------------------------------------");
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  DLOG_INFO(MOD_EB, "MemBufferReadWriteCorrectTest Write Start,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    mb->WriteUInt32(i);
  }
  DLOG_INFO(MOD_EB, "MemBufferReadWriteCorrectTest Write Done,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  //for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
  //  uint32 data = 0;
  //  // 这里面读取数据特别慢，时间都花在定位数据块上面了，必须要优化这个过程
  //  mb->CopyUInt32(i * sizeof(uint32), &data);
  //  if (data != i) {
  //    DLOG_ERROR(MOD_EB, "Data error ");
  //    return ;
  //  }
  //}
  DLOG_INFO(MOD_EB, "MemBufferReadWriteCorrectTest Read Start,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    uint32 data = 0;
    mb->ReadUInt32(&data);
    if (data != i) {
      DLOG_ERROR(MOD_EB, "Data error ");
      return ;
    }
  }
  DLOG_INFO(MOD_EB, "MemBufferReadWriteCorrectTest Read Done,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
}

void NormalMemorySpeedTest() {
  DLOG_INFO(MOD_EB, "--------------------------------------------------------");
  char *buffer = new char[TEST_DATA_SIZE * sizeof(uint32)];
  DLOG_INFO(MOD_EB, "NormalMemorySpeedTest Star Write");
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    memcpy(buffer + (i * sizeof(uint32)), (const char *)(&i), sizeof(uint32));
  }
  DLOG_INFO(MOD_EB, "NormalMemorySpeedTest End Write");
  //for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
  //  uint32 data = 0;
  //  // 这里面读取数据特别慢，时间都花在定位数据块上面了，必须要优化这个过程
  //  mb->CopyUInt32(i * sizeof(uint32), &data);
  //  if (data != i) {
  //    DLOG_ERROR(MOD_EB, "Data error ");
  //    return ;
  //  }
  //}
  DLOG_INFO(MOD_EB, "NormalMemorySpeedTest Start Read");
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    uint32 data = 0;
    memcpy((void *)&data, buffer + (i * sizeof(uint32)), sizeof(uint32));
    if (data != i) {
      DLOG_ERROR(MOD_EB, "Data error ");
      return ;
    }
  }
  DLOG_INFO(MOD_EB, "NormalMemorySpeedTest End Read");
}

void MembufferRawReadTest() {
  DLOG_INFO(MOD_EB, "--------------------------------------------------------");
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  DLOG_INFO(MOD_EB, "MembufferRawReadTest Write Start,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());

  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    mb->WriteUInt32(i);
  }


  DLOG_INFO(MOD_EB, "MembufferRawReadTest Write Done,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  DLOG_INFO(MOD_EB, "MembufferRawReadTest Read Start,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
  vzes::BlocksPtr blocks = mb->blocks();
  int index = 0;
  for (vzes::BlocksPtr::iterator iter = blocks.begin();
       iter != blocks.end(); iter++) {
    const char *data = (const char *)((*iter)->buffer);
    uint32 data_size = (*iter)->buffer_size / 4;
    for (int i = 0; i < data_size; i++, index++) {
      uint32 c = 0;
      memcpy((void *)&c, data + (i * sizeof(uint32)), sizeof(uint32));
      if (c != index) {
        DLOG_ERROR(MOD_EB, "Data error ");
        return ;
      }
    }
  }
  DLOG_INFO(MOD_EB, "MembufferRawReadTest Read Done,size:%d,blockSize:%d",
            mb->size(), mb->BlocksSize());
}

void MemBufferReadTest2() {
  DLOG_INFO(MOD_EB, "--------------------------------------------------------");
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb1 = vzes::MemBuffer::CreateMemBuffer();
  vzes::MemBuffer::Ptr mb2 = vzes::MemBuffer::CreateMemBuffer();
  vzes::MemBuffer::Ptr mb3 = vzes::MemBuffer::CreateMemBuffer();
  std::string str1, str2, str3, strtmp;
  const int half = 26;
  for (int i = 0; i < 2 * half; i++) {
    const char ch = (i % 26 + 'a');
    str1 += ch;
    if (i < half) {
      str2 += ch;
    } else {
      str3 += ch;
    }
  }
  mb1->WriteString(str1);
  //mb2->WriteString(str2);
  //mb3->WriteString(str3);
  mb1->CopyBuffer(mb2, half);
  mb1->ReadBuffer(mb3, half);
  str1 = mb1->ToString();
  str2 = mb2->ToString();
  str3 = mb3->ToString();
  printf("%s %s\n", str2.c_str(), str3.c_str());
  if (str2 != str3) {
    DLOG_INFO(MOD_EB, "MemBufferReadTest2 Error1.");
  }
  strtmp = str2;
  mb2->Clear();
  mb3->Clear();
  mb1->CopyBuffer(mb2, half);
  mb1->CopyBuffer(mb3, half);
  str1 = mb1->ToString();
  str2 = mb2->ToString();
  str3 = mb3->ToString();
  printf("%s %s\n", str2.c_str(), str3.c_str());
  if (str2 != str3) {
    DLOG_INFO(MOD_EB, "MemBufferReadTest2 Error2.");
  }
  printf("%s %s\n", str1.c_str(), strtmp.c_str());
  if (str1 != strtmp + str2) {
    DLOG_INFO(MOD_EB, "MemBufferReadTest2 Error3.");
  }
}

int main(void) {
  // Initialize the logging system
  (void)Log_Init(false);
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  MemBufferReadWriteBigTest();
  MemBufferReadWriteCorrectTest();
  NormalMemorySpeedTest();
  MembufferRawReadTest();
  MemBufferReadTest2();
  return EXIT_SUCCESS;
}
