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

#ifndef __WATCH_DOG_C_H__
#define __WATCH_DOG_C_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


// 看门狗模块默认配置文件路径定义,配置文件规则：
// http://note.youdao.com/groupshare/?token=0ADD9E8AADBF4AEBA70AF75D27148342&gid=31699737
#if defined WIN32
#define WDG_DEF_MODULE_FILE    "c:\\vz_cfg\\wdg_module.cfg"
#elif defined LITEOS
#define WDG_DEF_MODULE_FILE    "/config_file/wdg_module.cfg"
#elif defined UBUNTU64
#define WDG_DEF_MODULE_FILE    "/tmp/wdg_module.cfg"
#else
#define WDG_DEF_MODULE_FILE    "/tmp/app/exec/wdg_module.cfg"
#endif

#define WDG_DEF_FEEDDOG_TIME   (4)				// 默认喂狗时间(单位:秒)
#define WDG_DEF_TIMEOUT        (21)				// 看门狗默认超时时间(单位:秒)
#define WDG_MAX_TIMEOUT        (61)				// 看门狗最大超时时间(单位:秒)
#define WDG_MIN_TIMEOUT        (4)				// 看门狗最小超时时间(单位:秒)
#define WDG_MAX_PREREBOOT_CB_TIMEOUT  (3*1000)  // peRebootCb最大超时时间

// pre reboot回调函数
typedef void(*preRebootCb) (void);


// 看门狗模块初始化，VzEventBase内部调用
// return 成功: 0, 失败: -1
int WatchDog_Init(void);

// 看门狗模块销毁，VzEventBase内部调用
void WatchDog_Deinit(void);

// 看门狗模块注册,对于已注册的模块可通过该接口修改超时时间，
// 返回的key值不变。支持多线程并发
// name:模块名称，最大长度32Byte，超长按32Byte截断
// sec_timeout:模块看门狗超时时间，单位:秒，最大值为WDG_MAX_TIMEOUT
// return 成功: > 0，模块Key值; 失败: -1
int WatchDog_Register(const char *name, unsigned int sec_timeout);

// 设置用户回调函数，看门狗超时重启系统前回调用户，用于输出用户自定义信息
// usr_cb: 用户自定义的回调函数
// timeout:回调函数超时时长，单位毫秒,最大时间WDG_MAX_PREREBOOT_CB_TIMEOUT
int WatchDog_SetPreRebootCb(preRebootCb usr_cb, unsigned int timeout);

// 模块喂狗,支持多线程并发
// key:模块Key值，在WatchDog_Register时返回
// return 成功: 0, 失败: -1
int WatchDog_FeedDog(unsigned int key);

// 看门狗动态开关，默认为开启状态。
// 该接口只用于调试使用，正常版本不允许关闭狗
// enable : 1 打开， 0  关闭
// 设置成功:0  设置失败: -1
int WatchDog_Enable(int enable);

// 重启系统
// WIN32:exit app; Linux device:reboot system.
void WatchDog_RebootSystem(void);

// 打印已注册模块信息
void WatchDog_Dump(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __WATCH_DOG_C_H__
