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