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
#ifndef LIBAPP_MEM_DUMP_H_
#define LIBAPP_MEM_DUMP_H_

#include <stdio.h>

//#define MEM_TEST
#ifdef MEM_TEST

#include <iostream>
#include <string.h>
#include <malloc.h>

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <ImageHlp.h>
#pragma comment(lib, "DbgHelp.lib")
#else
#include <time.h>
#include <pthread.h>
#endif


#include "eventservice/base/criticalsection.h"
#include "eventservice/base/timeutils.h"
#include "eventservice/event/thread.h"

#define FILE_PATH  "d:\\vzmeminfo.txt"
#define MODULE_NAME_LEN (16)
#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif // End of #ifdef __cplusplus

enum {
  TO_FILE  = 0x01,    // 写入文件
  TO_POINT = 0x02,    // 打印到屏幕
};
typedef unsigned char MEM_DUMP_END_E;

enum {
  TOTAL   = 0x01,     // 总体
  DETAIL  = 0x02,     // 详细
};
typedef unsigned char MEM_DUMP_TYPE_E;

// 输出内存分配情况
// end: 输出终端，TO_FILE:写入文件，TO_POINT:打印到屏幕
// type: 输出方式，TOTAL:输出每个线程使用情况，DETAIL:输出所有内存块详细信息
// 选择输出到文件且输出所有内存块详细信息时会遍历所有的内存块，比较耗时。
extern void DumpMemInfo(const MEM_DUMP_END_E end, const MEM_DUMP_TYPE_E type);

// 输出单个线程内存分配情况
// thread_id : 线程id
// end: 输出终端，TO_FILE:写入文件，TO_POINT:打印到屏幕
// type: 输出方式，TOTAL:输出该线程的整体使用情况，DETAIL:输出该线程所有内存块详细信息
// 线程id每次都会改变，在线程名获取不了的情况下，需要每一次先打印出task信息
extern void DumpSingleThreadMemInfo(const unsigned int thread_id,
                                const MEM_DUMP_END_E end, const MEM_DUMP_TYPE_E type);

void *own_malloc(unsigned int nSize, char *pFile, unsigned int nLine);
void *own_realloc(void *pPtr, unsigned int nSize, char *pFile,
                  unsigned int nLine);
void own_free(void *pPtr, char *pFile, unsigned int nLine);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif // End of #ifdef __cplusplus

#define VZ_MALLOC(n)      own_malloc((n), (char *)__FILE__, __LINE__)
#define VZ_REALLOC(p, n)  own_realloc((p), (n), (char *)__FILE__, __LINE__)
#define VZ_FREE(p)        own_free((p), (char *)__FILE__, __LINE__)

#if 1
#ifdef __cplusplus

#ifdef WIN32
#define _THROW
#define _NOEXCEPT
#elif  LITEOS
#define _THROW 
#define _NOEXCEPT noexcept
#else 
#define _THROW throw(std::bad_alloc)
#define _NOEXCEPT throw()
#endif


void* operator new(unsigned int n, char *file, int line) _THROW;
void* operator new[](unsigned int n, char *file, int line) _THROW;

void  operator delete(void* p) _NOEXCEPT;
void  operator delete[](void* p) _NOEXCEPT;

void* operator new(unsigned int n) _THROW;
void* operator new[](unsigned int n) _THROW;

#define VZ_NEW        new(__FILE__, __LINE__)
#define VZ_DELETE     delete

#endif // End of #ifdef __cplusplus
#endif // End of #ifdef LITEOS 

#else

#define VZ_MALLOC      malloc
#define VZ_REALLOC     realloc
#define VZ_FREE        free
#define VZ_NEW         new
#define VZ_DELETE      delete

#endif

#endif // LIBAPP_MEM_DUMP_H_
