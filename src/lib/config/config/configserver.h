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

#ifndef __CONFIG_SERVER_H__
#define __CONFIG_SERVER_H__

#include "config/config/configclient.h"
#include <list>
#include "tinyxml2/tinyxml2.h"

namespace config {

#ifdef WIN32
#define CONFIG_PARENT_FOLD   "D:\\configdb"
#elif defined LITEOS
#define CONFIG_PARENT_FOLD   "/usr/configdb"
#elif defined UBUNTU64
#define CONFIG_PARENT_FOLD   "/home/poilk/Documents/kvdb"
#else
#define CONFIG_PARENT_FOLD   "/mnt/usr/configdb"
#endif

char *const module_property_folders[] = {
  "kvdb",
  "skvdb",
  "sskvdb",
  "ssskvdb"
};

//CACHE_SIZE最小值必须大于0
#define CONFIG_CACHE_SIZE 20

struct ConfigData {
  vzstd::string name;
  int index;
  vzstd::string min_value;
  vzstd::string max_value;
  vzstd::string default_value;
  vzstd::string value;
  tinyxml2::XMLElement *element;
  ConfigParamType para_type;
};

class ConfigServer {
 public:
  typedef boost::shared_ptr<ConfigServer> Ptr;
  typedef boost::shared_ptr<tinyxml2::XMLDocument> XMLDocumentPtr;
  ConfigServer(vzstd::string module_name,
               CFG_MODULE_PROPERTY module_property);
  ~ConfigServer() {
  };

 public:
  static int GetCfgModules(std::vector<vzstd::string> modules,
                           CFG_MODULE_PROPERTY module_property );
  int GetCfgValByName(const vzstd::string &param_name, void *val, int val_len);
  int GetCfgValByName(const vzstd::string &param_name, vzstd::string &val);
  int GetCfgValById(int param_index, void *val, int val_len);
  int GetCfgValById(int param_index, vzstd::string &val);
  int GetCfgVal(ConfigParamNode &config);
  int GetAllCfgVal(std::vector<ConfigParamNode> &configs);
  int GetArrayCfgVal(std::vector<ConfigParamNode> &configs);

  int SetCfgValByName(const vzstd::string &param_name,
                      const void *val, int val_len);
  int SetCfgValByName(const vzstd::string &param_name,
                      const vzstd::string &val);
  int SetCfgValById(int param_index, const void *val, int val_len);
  int SetCfgValById(int param_index, const vzstd::string &val);
  int SetCfgVal(const std::vector<ConfigParamNode> &configs,
                bool useCallBack = false);
  bool SetPreCheckValFunc(PreCheckCfgValFunc pFunc, void *pUserData);

  int ToString(vzstd::string &val);
  int Init();
  bool ResetXMLDoc();

  vzstd::string GetModuleName() {
    return module_name_;
  }

  CFG_MODULE_PROPERTY GetModuleProperty() {
    return module_property_;
  }

 private:
  static int OpenXMLDoc(XMLDocumentPtr &xml_doc,
                        const vzstd::string &file_path);
  int InitXMLDoc();
  int GetCfgVal(const ConfigData &data,
                void *val, int len,
                bool use_default = true);
  int GetCfgVal(const ConfigData &data,
                vzstd::string &val,
                bool use_default = true);
  int GetArrayCfgVal(const ConfigData &data,
                     std::vector<ConfigParamNode> &configarrs);
  int GetCfgVal(const ConfigData &data,
                ConfigParamNode &config,
                bool use_default = true);
  int SetCfgVal(const ConfigData &data,
                const ConfigParamNode &config,
                bool write_to_file = true);
  int SetArrayCfgVal(const ConfigData &data,
                     const std::vector<ConfigParamNode> &configs);
  bool SaveXMLDoc();
  bool Inited() {
    return xml_doc_.get() != NULL;
  }
  int LoadXMLAttrValue(const tinyxml2::XMLElement *element,
                       const char *attribute_name,
                       vzstd::string &value);
  int LoadXMLEleData(const vzstd::string &param_name, ConfigData &data);
  int LoadXMLEleData(int param_index, ConfigData &data);
  int LoadXMLEleData(tinyxml2::XMLElement *element,
                     ConfigData &data);
  int ConvertStringToConfigParamType(const vzstd::string &str,
                                     ConfigParamType &type);

  vzes::CriticalSection crit_;
  vzstd::string file_path_, file_path_bak_;
  vzstd::string module_name_;
  CFG_MODULE_PROPERTY module_property_;
  PreCheckCfgValFunc call_back_;
  void *call_back_user_data_;
  XMLDocumentPtr xml_doc_;
};

class ConfigServerManager {
 public:
  ConfigServer::Ptr GetConfigServer(vzstd::string module_name,
                                    CFG_MODULE_PROPERTY module_property);
  static ConfigServerManager *GetCfgSrvManIns();
 private:
  typedef std::list<ConfigServer::Ptr> ConfigServerPtrs;
  vzes::CriticalSection crit_;
  ConfigServerPtrs config_servers_;
};
} //namespace config

#endif //__CONFIG_SERVER_H__
