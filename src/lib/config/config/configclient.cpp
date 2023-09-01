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

#include "config/config/configclient.h"
#include "config/config/configserver.h"

namespace config {

class ConfigClientImpl : public ConfigClient {
 public:

  ConfigClientImpl() {
  }
  ~ConfigClientImpl() {
  }
  virtual int GetCfgValByName(
    const std::string &param_name,
    void *val,
    int val_len,
    const std::string &module_name,
    CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->GetCfgValByName(param_name, val, val_len);
    }
  }
  virtual int GetCfgValByName(const std::string &param_name,
                              std::string &val,
                              const std::string &module_name,
                              CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->GetCfgValByName(param_name, val);
    }
  }

  virtual int GetCfgValById(int param_index, void *val, int val_len,
                            const std::string &module_name,
                            CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->GetCfgValById(param_index, val, val_len);
    }
  }
  virtual int GetCfgValById(int param_index, std::string &val,
                            const std::string &module_name,
                            CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->GetCfgValById(param_index, val);
    }
  }

  virtual int SetCfgValByName(const std::string &param_name,
                              const void *val, int val_len,
                              const std::string &module_name,
                              CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->SetCfgValByName(param_name, val, val_len);
    }
  }

  virtual int SetCfgValByName(const std::string &param_name,
                              const std::string &val,
                              const std::string &module_name,
                              CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->SetCfgValByName(param_name, val);
    }
  }

  virtual int SetCfgValById(int param_index, const void *val, int val_len,
                            const std::string &module_name,
                            CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->SetCfgValById(param_index, val, val_len);
    }
  }
  virtual int SetCfgValById(int param_index, const std::string &val,
                            const std::string &module_name,
                            CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->SetCfgValById(param_index, val);
    }
  }
  virtual int SetCfgVal(const std::vector<ConfigParamNode> &configs,
                        const std::string &module_name,
                        CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->SetCfgVal(configs, true);
    }
  }

  virtual int SetCfgVal(const ConfigParamNode &config,
                        const std::string &module_name,
                        CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      std::vector<ConfigParamNode> configs;
      configs.push_back(config);
      return cfg_srv->SetCfgVal(configs, false);
    }
  }

  virtual int GetAllCfgVal(std::vector<ConfigParamNode> &configs,
                           const std::string &module_name,
                           CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      configs.clear();
      return cfg_srv->GetAllCfgVal(configs);
    }
  }

  virtual int GetArrayCfgVal(
    std::vector<ConfigParamNode> &configs,
    const std::string &module_name,
    CFG_MODULE_PROPERTY module_property = CFG_M_SEC_NOR) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return CFG_OP_FILE_ERR;
    } else {
      return cfg_srv->GetAllCfgVal(configs);
    }
  }

  virtual bool SetPreCheckValFunc(PreCheckCfgValFunc p_func, void *p_user_data,
                                  const std::string &module_name,
                                  CFG_MODULE_PROPERTY module_property) {
    ConfigServerManager *cfg_man_ins = ConfigServerManager::GetCfgSrvManIns();
    ConfigServer::Ptr cfg_srv =
      cfg_man_ins->GetConfigServer(module_name, module_property);
    if (!cfg_srv.get()) {
      return false;
    } else {
      return cfg_srv->SetPreCheckValFunc(p_func, p_user_data);
    }
  }
};

ConfigClient::Ptr ConfigClient::CreateConfigClient() {
  return ConfigClientImpl::Ptr(new ConfigClientImpl());
};

} //namespace config