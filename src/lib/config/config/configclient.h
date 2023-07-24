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

#ifndef __CONFIG_CLIENT_H__
#define __CONFIG_CLIENT_H__

#include <vector>
#include "astl/include/string.hpp"
#include "eventservice/base/basicincludes.h"

namespace config {

#define CFG_PARAM_TYPE_MIN_VAL 0
#define CFG_PARAM_TYPE_MAX_VAL 6
enum ConfigParamType {
  PARA_TYPE_USER_DEFINE = 0,
  // 自定义类型，基础库不对此类型做校验，由各模块自己保证参数合法性
  PARA_TYPE_INT32 = 1,
  PARA_TYPE_DOUBLE = 2,
  PARA_TYPE_FLOAT = 3,
  PARA_TYPE_INT64 = 4,
  PARA_TYPE_STRING = 5,
  PARA_TYPE_ARRARY = 6
};

struct ConfigParamNode {
  ConfigParamNode() {}
  //set_by_id construct
  ConfigParamNode(const int idx,
                  const ConfigParamType tp,
                  const vzstd::string &v) :
    index(idx), key(""), type(tp), val(v) {
  }
  //set_by_key construct
  ConfigParamNode(const vzstd::string &k,
                  const ConfigParamType tp,
                  const vzstd::string &v) :
    index(0), key(k), type(tp), val(v) {
  }
  //get_by_key construct
  ConfigParamNode(const vzstd::string &k,
                  const ConfigParamType tp):
    index(0), key(k), type(tp) {
  }
  //get_by_id construct
  ConfigParamNode(const int idx,
                  const ConfigParamType tp) :
    index(idx), key(""), type(tp) {
  }
  int index;
  vzstd::string key;
  ConfigParamType type;
  vzstd::string val;
};

enum CFG_MODULE_PROPERTY {
  CFG_M_SEC_NOR = 0,         // 普通参数，对应当前的KVDB
  CFG_M_SEC_ONE = 1,         // 一级敏感参数（网络参数），对应当前的SKVDB
  CFG_M_SEC_TWO = 2,         // 二级敏感参数（登录密码，加密方式），对应当前的SKVDB
  CFG_M_SEC_THREE = 3,       // 三级敏感参数（用户数据）, 对应当前的USERDATA_KVDB
};

enum CFG_OPERATE_STATUS {
  CFG_OP_SUCCEED = 200,               // 操作成功
  CFG_OP_NOT_FOUND = 404,             // 未找到对应数据
  CFG_OP_RANGE_ERR = 422,             // 参数范围非法
  CFG_OP_TYPE_ERR = 423,              // 参数类型错误
  CFG_OP_PARALEN_ERR = 424,           // 参数长度错误
  CFG_OP_INNER_ERR = 500,             // 内部错误
  CFG_OP_FILE_ERR = 501,              // 文件格式错误
  CFG_OP_NO_VALUE = 502,			  // 无值
  CFG_OP_USER_CHECK_ERR = 503         // 用户校验
};

#define GROUP_NAME_MAX_LEN 30
#define FLOAT_EPS (double)(1e-9)

typedef int (*PreCheckCfgValFunc)(vzstd::string groupName,
                                  std::vector<ConfigParamNode> paras,
                                  void *userData);

class ConfigClient {
 public:
  virtual ~ConfigClient() {};
  typedef boost::shared_ptr<ConfigClient> Ptr;
  static ConfigClient::Ptr
  CreateConfigClient();

  virtual int GetCfgValByName(
    const vzstd::string &param_name,
    void *val,
    int val_len,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int GetCfgValByName(
    const vzstd::string &param_name,
    vzstd::string &val, const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int GetCfgValById(
    int param_index,
    void *val,
    int val_len,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int GetCfgValById(
    int param_index,
    vzstd::string &val,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgValByName(
    const vzstd::string &param_name,
    const void *val,
    int val_len,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgValByName(
    const vzstd::string &para_name,
    const vzstd::string &val,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgValById(
    int param_index,
    const void *val,
    int val_len,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgValById(
    int param_index,
    const vzstd::string &val,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgVal(
    const std::vector<ConfigParamNode> &configs,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int SetCfgVal(
    const ConfigParamNode &config,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int GetAllCfgVal(
    std::vector<ConfigParamNode> &configs,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual int GetArrayCfgVal(
    std::vector<ConfigParamNode> &configs,
    const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;

  virtual bool SetPreCheckValFunc(
    PreCheckCfgValFunc p_func,
    void *p_user_data, const vzstd::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) = 0;
}; //ConfigClient

} //namespace config
#endif  // __CONFIG_CLIENT_H__
