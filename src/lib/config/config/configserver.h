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
  std::string name;
  int index;
  std::string min_value;
  std::string max_value;
  std::string default_value;
  std::string value;
  tinyxml2::XMLElement *element;
  ConfigParamType para_type;
};

class ConfigServer {
 public:
  typedef boost::shared_ptr<ConfigServer> Ptr;
  typedef boost::shared_ptr<tinyxml2::XMLDocument> XMLDocumentPtr;
  ConfigServer(std::string module_name,
               CFG_MODULE_PROPERTY module_property);
  ~ConfigServer() {
  };

 public:
  static int GetCfgModules(std::vector<std::string> modules,
                           CFG_MODULE_PROPERTY module_property );
  int GetCfgValByName(const std::string &param_name, void *val, int val_len);
  int GetCfgValByName(const std::string &param_name, std::string &val);
  int GetCfgValById(int param_index, void *val, int val_len);
  int GetCfgValById(int param_index, std::string &val);
  int GetCfgVal(ConfigParamNode &config);
  int GetAllCfgVal(std::vector<ConfigParamNode> &configs);
  int GetArrayCfgVal(std::vector<ConfigParamNode> &configs);

  int SetCfgValByName(const std::string &param_name,
                      const void *val, int val_len);
  int SetCfgValByName(const std::string &param_name,
                      const std::string &val);
  int SetCfgValById(int param_index, const void *val, int val_len);
  int SetCfgValById(int param_index, const std::string &val);
  int SetCfgVal(const std::vector<ConfigParamNode> &configs,
                bool useCallBack = false);
  bool SetPreCheckValFunc(PreCheckCfgValFunc pFunc, void *pUserData);

  int ToString(std::string &val);
  int Init();
  bool ResetXMLDoc();

  std::string GetModuleName() {
    return module_name_;
  }

  CFG_MODULE_PROPERTY GetModuleProperty() {
    return module_property_;
  }

 private:
  static int OpenXMLDoc(XMLDocumentPtr &xml_doc,
                        const std::string &file_path);
  int InitXMLDoc();
  int GetCfgVal(const ConfigData &data,
                void *val, int len,
                bool use_default = true);
  int GetCfgVal(const ConfigData &data,
                std::string &val,
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
                       std::string &value);
  int LoadXMLEleData(const std::string &param_name, ConfigData &data);
  int LoadXMLEleData(int param_index, ConfigData &data);
  int LoadXMLEleData(tinyxml2::XMLElement *element,
                     ConfigData &data);
  int ConvertStringToConfigParamType(const std::string &str,
                                     ConfigParamType &type);

  vzes::CriticalSection crit_;
  std::string file_path_, file_path_bak_;
  std::string module_name_;
  CFG_MODULE_PROPERTY module_property_;
  PreCheckCfgValFunc call_back_;
  void *call_back_user_data_;
  XMLDocumentPtr xml_doc_;
};

class ConfigServerManager {
 public:
  ConfigServer::Ptr GetConfigServer(std::string module_name,
                                    CFG_MODULE_PROPERTY module_property);
  static ConfigServerManager *GetCfgSrvManIns();
 private:
  typedef std::list<ConfigServer::Ptr> ConfigServerPtrs;
  vzes::CriticalSection crit_;
  ConfigServerPtrs config_servers_;
};
} //namespace config

#endif //__CONFIG_SERVER_H__
