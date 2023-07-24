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

#ifndef __LOG_CLIENT_C_H__
#define __LOG_CLIENT_C_H__

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define LOG_RELEASE_MAX_LEN   (256)   // Release日志的最大长度(Byte),包括文件名、行号
#define LOG_DEBUG_MAX_LEN     (4096)  // Debug日志的最大长度(Byte),包括文件名、行号
#define LOG_MODULE_MAX_NUM    (16)    // Debug日志模块的最大数量
#define LOG_MODULE_NAME_LEN   (16)    // Debug日志模块名称最大长度
#define LOG_MODULE_ID_BASE    (2001)  // Debug模块ID的基础值
#define MOD_EB                (LOG_MODULE_ID_BASE) // 基础库Debug日志模块ID

#define  LOG_ID_WATCHDOG_REBOOT     2401  // watchdog重启
#define  LOG_ID_CRASHED_SIGSEGV     2410  // app程序崩溃/SIGSEGV

// 日志类型
// - 枚举类型组合使用
typedef enum {
  // Debug日志类型
  LT_DEBUG = 0x01,  // 调试日志
  // Release日志类型
  LT_POINT = 0x02,  // 业务流程日志
  LT_SYS   = 0x04,  // 系统日志
  LT_DEV   = 0x08,  // 设备日志
  LT_UI    = 0x10,  // 用户操作日志
} LOG_TYPE_E_ITEM;
typedef unsigned char LOG_TYPE_E;

// Debug日志级别
typedef enum {
  LL_DEBUG,
  LL_INFO,
  LL_WARNING,
  LL_ERROR,
  LL_KEY,           // 用于关键流程日志打印，!!!该类型日志尽量少
  LL_NONE,          // 不输出任何级别的日志
} LOG_LEVEL_E;

// 日志输出终端，适用于Debug和Release日志，默认值分别为:
// - Debug日志: LE_LOCAL
// - Release日志: LE_OFF
// - 枚举类型组合使用
typedef enum {
  LE_OFF      = 0x00,   // 无操作
  LE_LOCAL    = 0x01,   // 打印到本地控制台
  LE_REMOTE   = 0x02,   // 发送到远端日志服务器
  LE_FILE     = 0x04,   // 保存到文件
} LOG_PRINT_END_E_ITEM;
typedef unsigned char LOG_PRINT_END_E;

#pragma pack(1)
// 日志时间，格林威治时间，从1970年1月1日零点零分零秒开始计时
typedef struct {
  uint32  sec;      // 秒,从1970年1月1日0时0分0秒至今的UTC时间所经过的秒数
  uint32  usec;     // 微秒 */
} LOG_TIME_S;

// 一级缓存节点信息
typedef struct {
  uint16  length;   // 整条记录长度: LOG_NODE_S + body
  uint8   type;     // 日志类型 (LOG_TYPE_E)
  uint8   level;    // Debug日志级别，Release日志忽略
  uint32  sec;      // 日志产生时间:秒
  uint32  usec;     // 日志产生时间:微秒
  //uint8 body[0];  // 日志数据指针，存储结构为:trace data
} LOG_NODE_S;

// 文件节点存储信息
typedef struct {
  uint8      head;    // 日志节点头标识
  uint32     id;      // 日志记录自增id
  LOG_NODE_S node;    // 日志信息
  //uint16   length;  // 整条记录长度
} LOG_NODE_FILE_S;

typedef enum {
  LOG_QUERY_ASC_TYPE,
  LOG_QUERY_DESC_TYPE
} LOG_QUERY_TYPE;

typedef struct {
  bool is_first;          //第一次调用设为true，之后查询设为false.
  LOG_QUERY_TYPE qtype;   //查询的类型，顺序或者逆序
  uint32 min_id;          //第一次查询后会写入min_id,之后查询需要传入此参数
  uint32 max_id;          //第一次查询后会写入min_id,之后查询需要传入此参数
  uint32 start_id;        //查询开始的id,非第一次查询时会读取此值
  uint32 last_id;         //本次查询的最后一条记录，下一次查询时作为start_id
} LOG_QUERY_NODE;

#pragma pack()


// 日志模块初始化
// - 通过App框架创建的进程，在基础库内部会调用该接口初始化日志模块，应用不需要调用。
// - 不是通过App框架创建的进程，需要应用自己调用改接口初始化日志模块。
// multi_process:是否支持跨进程日志，以Server、Client模式运行；
// - true:支持跨进程日志，通过该接口初始化的进程都可以做为server，以最先启动的进程
//   为Server，其他进程为client，Client端的日志在Server端存储、显示。
// - false:不支持跨进程日志，该进程的日志在本进程中存储、显示。
// return 成功:true; 失败:false
bool Log_Init(bool multi_process);

// 日志Client模块初始化。该接口为Server、Client模式下，Client端初始化接口，
// 用于跨进程日志传输，如果Server端未创建，则初始化失败。
bool Log_ClientInit(void);

// 日志模块去初始化，基础库内部调用
void Log_Deinit(void);

// 设置日志远端服务器ip地址信息
// ip:ip地址，主机字节序
// port: 端口，主机字节序
// return 成功:true; 失败:false
bool Log_SetRemoteServer(uint32 ip, uint16 port);

// 设置所有类型日志输出的终端设备，即打印到本地或者发送到远端服务器。
// - Release日志：默认只写入到文件，可通过该接口配置是否输出到屏幕
//   或远端服务器。
// - Debug日志：默认输出到屏幕，可通过该接口配置是否输出到屏幕
//   或远端服务器或写入文件。
// type:日志类型,LOG_TYPE_E
// print_end: 终端设备, LOG_PRINT_END_E_ITEM
// return 成功:true; 失败:false
bool Log_SetPrintEnd(int type, int print_end);

// 设置所有日志类型同步打印
// 注意此接口为调试用接口，一般不要使用,开启此功能后，包含了要打印到
// 本地的日志类型都只会打印到本地，不能同时写入文件和发送到远端。
// - on: 同步打印开关
// return 成功:true; 失败:false;
bool Log_SetSyncPrint(bool on);

// 刷新缓存中的日志数据到文件中，运行在调用线程
void Log_FlushCache(void);

// 打印缓存信息
void Log_DumpChacheInfo();

// 注册Debug日志模块，最多可以注册LOG_MODULE_MAX_NUM个，name标识一个
// 唯一的模块,如果是已经注册过的模块，则返回该模块对应的id
// name: 模块名称，标识一个唯一的模块,最大长度LOG_MODULE_NAME_LEN，超长截断
// 有效值: "0～9"、"A～Z"、"a～z"、"_"（下划线）
// level:日志级别。也可以通过Log_SetLevel配置, LOG_LEVEL_E
// return 成功:>0,模块对应的ID; 失败:-1
int Log_DbgModRegist(const char *name, int level);

// 设置Debug日志级别
// mod_id:模块id，通过Log_ModRegister返回。
// 如果mod_id为0xFF则对所有的模块配置
// level: 级别, LOG_LEVEL_E
// return 成功:true; 失败:false
bool Log_DbgSetLevel(int mod_id, int level);

// 输出Debug类型的日志，打印到屏幕或者发送到服务器
// mod_id:模块id
// level:日志级别, LOG_LEVEL_E
// file:输出日志的当前文件的名称
// line:所在文件中的行号
void Log_DbgTrace(int mod_id, int level, char* file,
                  uint32 line, char *format, ...);

// 设置Release类型日志的二级缓存大小、缓存超时时间，
// - 缓存大小：可缓存日志的条数,缓存满后将日志写入文件
// - 缓存超时时间：超时后将缓存中的日志写入文件
// type:日志类型, LOG_TYPE_E_ITEM,可取值LT_POINT、LT_SYS、LT_DEV、LT_UI
// cache_size:缓存大小，单位:块
// time_out:缓存超时时间，单位:秒
bool Log_RelCacheCfg(int type, uint16 cache_size, uint16 time_out);

// 输出Release类型的日志，内容为ID映射+自定义的字符串，
// 输出内容格式为:id_1,id_2,user formated string
// 用户读取该类型的日志后，需要按该格式解析id_1、id_2。
// type:日志类型,LOG_TYPE_E_ITEM
// id_1:id 1
// id_2:id 2
// format:格式化字符串
void Log_RelTraceMap2(int type, uint16 id_1, uint16 id_2,
                      char *format, ...);

// 打点日志输出宏定义
#ifndef LOG_POINT
#define LOG_POINT(id_1, id_2, format, ...) \
  Log_RelTraceMap2(LT_POINT, id_1,id_2, (char*)format, ##__VA_ARGS__)
#endif

// 输出Release类型的日志，输出日志内容到对应的文件中，
// 输出内容格式为:user formated string
// type:日志类型, LOG_TYPE_E_ITEM, 可取值LT_SYS、LT_DEV、LT_UI
// format:格式化字符串
void Log_RelTrace(int type, char *format, ...);

// Release类型日志输出宏定义，日志类型可取值LT_SYS、LT_DEV、LT_UI
#ifndef LOG_TRACE
#define LOG_TRACE(type, format, ...) \
  Log_RelTrace(type, (char*)format, ##__VA_ARGS__)
#endif

// 查询Release类型日志
// type_mask:日志类型集合的mask，即LOG_TYPE_E中定义的Release日志类型按位或的值，
// 如: LT_POINT | LT_SYS | LT_UI
// start:起始时间, 可以通过TimeMkLocal转换
// start:截止时间, 可以通过TimeMkLocal转换
// size:查询的记录最大数量
// buffer:输出，查询结果输出内存，数组一维值为size，二维值为LOG_RELEASE_MAX_LEN
// 数据的结构为LOG_NODE_S，按此结构解析:
// - 打点类型日志:LOG_NODE_S + id_1,id_2,user string
// - 其他类型日志:LOG_NODE_S + user string
// return 查询到的结果数量
int Log_Search(const uint8 type_mask, const LOG_TIME_S start,
               const LOG_TIME_S end, const uint32 size,
               char buffer[][LOG_RELEASE_MAX_LEN], LOG_QUERY_NODE &qnode);

bool Log_SearchUseCustomFolder(const char folder[128], int len);

// Debug类型日志输出宏定义，简化用户接口调用
#ifndef DLOG_DEBUG
#define DLOG_DEBUG(mid, format, ...) \
  Log_DbgTrace(mid, LL_DEBUG, (char*)__FILE__, __LINE__, (char*)format, ##__VA_ARGS__)
#endif

#ifndef DLOG_INFO
#define DLOG_INFO(mid, format, ...) \
  Log_DbgTrace(mid, LL_INFO, (char*)__FILE__, __LINE__, (char*)format, ##__VA_ARGS__)
#endif

#ifndef DLOG_WARNING
#define DLOG_WARNING(mid, format, ...) \
  Log_DbgTrace(mid, LL_WARNING, (char*)__FILE__, __LINE__, (char*)format, ##__VA_ARGS__)
#endif

#ifndef DLOG_ERROR
#define DLOG_ERROR(mid, format, ...) \
  Log_DbgTrace(mid, LL_ERROR, (char*)__FILE__, __LINE__, (char*)format, ##__VA_ARGS__)
#endif

#ifndef DLOG_KEY
#define DLOG_KEY(mid, format, ...) \
  Log_DbgTrace(mid, LL_KEY, (char*)__FILE__, __LINE__, (char*)format, ##__VA_ARGS__)
#endif

// 以字符串形式打印内存数据到控制台
// discript:数据描述信息，可以为NULL
// data_buffer:数据指针
// 打印长度
void Log_HexDump(const char *discript, const unsigned char *data,
                 uint32 size);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // __LOG_CLIENT_C_H__
