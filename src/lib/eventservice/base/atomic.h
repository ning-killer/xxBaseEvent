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

#ifndef __LOG_ATOMIC_C_H__
#define __LOG_ATOMIC_C_H__

#if defined(WIN32)
#include <windows.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

// Windows InterLock系列原子操作函数，其参数务必4或8字节对齐，
// volatile long或者ulong，适用于: 1.共享内存变量类型；
// 2.资源变量类型；3.局部变量类型
#if defined(_MSC_VER)
#define VZ_BCAS(ptr, oldval, newval)  \
  ((InterlockedCompareExchange(ptr, newval, oldval) == oldval) ? true : false)

#define VZ_VCAS(ptr, oldval, newval)  \
  InterlockedCompareExchange(ptr, newval, oldval)

// Add a value to a given integer as an atomic operation and
// returns the original value of the given integer.
#define VZ_FAA(ptr, value)  \
  InterlockedExchangeAdd(ptr, value)

// Sub a value to a given integer as an atomic operation and
// returns the original value of the given integer.
#define VZ_FAS(ptr, value)  \
  InterlockedExchangeAdd(ptr, (0 - value))

#define VZ_FAOR(ptr, value)  \
  InterlockedOr(ptr, value)

#define VZ_FAAND(ptr, value)  \
  InterlockedAnd(ptr, value)

#define VZ_FAXOR(ptr, value)  \
  InterlockedXor(ptr, value)

#define VZ_FANAND(ptr, value)  \
  (*ptr = (~*ptr) & value)

#define VZ_AAF(ptr, value)  \
  (InterlockedExchangeAdd(ptr, value) + value)

#define VZ_SAF(ptr, value)  \
  (InterlockedExchangeAdd(ptr, (0 - value)) - value)

#define VZ_ORAF(ptr, value)  \
  (InterlockedOr(ptr, value) | value)

#define VZ_ANDAF(ptr, value)  \
  (InterlockedAnd(ptr, value) & value)

#define VZ_XORAF(ptr, value)  \
  (InterlockedXor(ptr, value) ^ value)

#define VZ_NANDAF(ptr, value)  \
  (*ptr = (~*ptr) & value)

#define VZ_LTAS(ptr, value)  \
  InterlockedExchange(ptr, value)

#define VZ_LR(ptr)  \
  InterlockedExchange(ptr, 0)

// GCC编译器原子操作函数定义，支持8、16、32、64字节操作数
#elif defined(__GNUC__)
// 原子的比较和交换，如果*ptr == oldval,就将newval写入*ptr,
// 在相等并写入的情况下返回true, 否则返回false
#define VZ_BCAS(ptr, oldval, newval)  \
  __sync_bool_compare_and_swap(ptr, oldval, newval)

// 原子的比较和交换，如果*ptr == oldval,就将newval写入*ptr,
// 无论是否相等，都返回操作之前的值
#define VZ_VCAS(ptr, oldval, newval)   \
  __sync_val_compare_and_swap(ptr, oldval, newval)

// 原子加，*ptr加上value，返回操作之前的*ptr，
// 即先取值，再执行加法操作
#define VZ_FAA(ptr, value)  \
  __sync_fetch_and_add(ptr, value)

// 原子减，*ptr减去value，返回操作之前的*ptr，
// 即先取值，再执行减法操作
#define VZ_FAS(ptr, value)   \
  __sync_fetch_and_sub(ptr, value)

// 原子或，*ptr按位或上value，返回操作之前的*ptr，
// 即先取值，再执行位或操作
#define VZ_FAOR(ptr, value)   \
  __sync_fetch_and_or(ptr, value)

// 原子与，*ptr按位与上value，返回操作之前的*ptr，
// 即先取值，再执行位与操作
#define VZ_FAAND(ptr, value)   \
  __sync_fetch_and_and(ptr, value)

// 原子异或，*ptr按位异或上value，返回操作之前的*ptr，
// 即先取值，再执行位异或操作
#define VZ_FAXOR(ptr, value)   \
  __sync_fetch_and_xor(ptr, value)

// 原子与非，*ptr按位与非上value，返回操作之前的*ptr，
// 即先取值，再执行位与非操作
#define VZ_FANAND(ptr, value)   \
  __sync_fetch_and_nand(ptr, value)

// 原子加，*ptr加上value，返回操作之后的*ptr，
// 即先执行加法操作，再取值
#define VZ_AAF(ptr, value)    \
  __sync_add_and_fetch(ptr, value)

// 原子减，*ptr减去value，返回操作之后的*ptr，
// 即先执行减法操作，再取值
#define VZ_SAF(ptr, value)    \
  __sync_sub_and_fetch(ptr, value)

// 原子或，*ptr按位或上value，返回操作之后的*ptr，
// 即先执行位或操作，再取值
#define VZ_ORAF(ptr, value)   \
  __sync_or_and_fetch(ptr, value)

// 原子与，*ptr按位与上value，返回操作之前的*ptr，
// 即先取值，再执行位与操作
#define VZ_ANDAF(ptr, value)    \
  __sync_and_and_fetch(ptr, value)

// 原子异或，*ptr按位异或上value，返回操作之后的*ptr，
// 即先执行位异或操作，再取值
#define VZ_XORAF(ptr, value)   \
  __sync_xor_and_fetch(ptr, value)

// 原子与非，*ptr按位与非上value，返回操作之后的*ptr，
// 即先执行位与非操作，再取值
#define VZ_NANDAF(ptr, value)   \
  __sync_nand_and_fetch(ptr, value)

// 将*ptr设为value并返回*ptr操作之前的值
#define VZ_LTAS(ptr, value)   \
  __sync_lock_test_and_set(ptr, value)

// 将*ptr置0
#define VZ_LR(ptr)    \
  __sync_lock_release(ptr)

#endif  // #if defined(_MSC_VER)



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __LOG_ATOMIC_C_H__
