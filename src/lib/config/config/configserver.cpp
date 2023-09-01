#include "config/config/configserver.h"
#include "log/log/log_client.h"

#define CFG_ATTR_NAME_ID				"id"
#define CFG_ATTR_NAME_TYPE				"type"
#define CFG_ATTR_NAME_DEFAULT			"default"
#define CFG_ATTR_NAME_MIN				"min"
#define CFG_ATTR_NAME_MAX				"max"
#define CFG_ATTR_NAME_DESCRIBE			"describe"
#define CFG_ATTR_NAME_ITEM				"item"
#define CFG_ATTR_NAME_ITEM_TYPE			"item_type"
#define CFG_ATTR_NAME_ITEM_MAX_SIZE		"item_max_size"

namespace config {

ConfigServer::ConfigServer(std::string module_name,
                           CFG_MODULE_PROPERTY module_property) {
  char buf[1000];
  if (module_name.length() > GROUP_NAME_MAX_LEN) {
    return;
  }
#ifdef WIN32
  sprintf(buf, "%s\\%s\\%s.xml", CONFIG_PARENT_FOLD,
          module_property_folders[(int) module_property],
          module_name.c_str());
  file_path_.append(buf);
  sprintf(buf, "%s\\%s\\%s_bak.xml", CONFIG_PARENT_FOLD,
          module_property_folders[(int) module_property],
          module_name.c_str());
  file_path_bak_.append(buf);
#else
  sprintf(buf, "%s/%s/%s.xml", CONFIG_PARENT_FOLD,
          module_property_folders[(int) module_property],
          module_name.c_str());
  file_path_.append(buf);
  sprintf(buf, "%s/%s/%s_bak.xml", CONFIG_PARENT_FOLD,
          module_property_folders[(int) module_property],
          module_name.c_str());
  file_path_bak_.append(buf);
#endif

  module_name_ = module_name;
  module_property_ = module_property;
  call_back_ = NULL;
  DLOG_INFO(MOD_EB, "Create ConfigServer done");
}

int ConfigServer::OpenXMLDoc(ConfigServer::XMLDocumentPtr &xml_doc,
                             const std::string &file_path) {
  xml_doc.reset(new tinyxml2::XMLDocument);
  if (file_path.length() == 0) {
    return CFG_OP_INNER_ERR;
  }
  int error = xml_doc->LoadFile(file_path.c_str());
  if (error) {
    DLOG_WARNING(MOD_EB, "Load XMLFile:\"%s\".error : %d",
                 file_path.c_str(), error);
    return CFG_OP_FILE_ERR;
  } else {
    DLOG_INFO(MOD_EB, "Init ConfigServer Succeed");
    return CFG_OP_SUCCEED;
  }
}

int ConfigServer::Init() {
  vzes::CritScope cr(&crit_);
  return InitXMLDoc();
}

int ConfigServer::InitXMLDoc() {
  int ret;
  if (xml_doc_.get() != NULL) {
    return CFG_OP_SUCCEED;
  }
  if ((ret = OpenXMLDoc(xml_doc_, file_path_)) != CFG_OP_SUCCEED) {
    if ((ret = OpenXMLDoc(xml_doc_, file_path_bak_)) != CFG_OP_SUCCEED) {
      DLOG_INFO(MOD_EB, "Init ConfigServer XMLdoc Failed");
      return ret;
    }
  }
  DLOG_INFO(MOD_EB, "Init ConfigServer XMLdoc Succeed");

  return CFG_OP_SUCCEED;
}

bool ConfigServer::ResetXMLDoc() {
  vzes::CritScope cr(&crit_);
  xml_doc_.reset();
  DLOG_INFO(MOD_EB, "Close ConfigServer XMLdoc Succeed");
  return true;
}

bool ConfigServer::SaveXMLDoc() {
  if (xml_doc_->SaveFile(file_path_.c_str())) {
    DLOG_ERROR(MOD_EB, "save xml file:\"%s\" error.", file_path_.c_str());
    return false;
  }
  if (xml_doc_->SaveFile(file_path_bak_.c_str())) {
    DLOG_ERROR(MOD_EB, "save xml file:\"%s\" error.", file_path_bak_.c_str());
    return false;
  }
  DLOG_INFO(MOD_EB, "save xml modle:\"%s\" succeed.", module_name_.c_str());
  return true;
}

bool ConfigServer::SetPreCheckValFunc(PreCheckCfgValFunc pFunc,
                                      void *pUserData) {
  vzes::CritScope cr(&crit_);
  call_back_ = pFunc;
  call_back_user_data_ = pUserData;
  return true;
}

int ConfigServer::LoadXMLAttrValue(const tinyxml2::XMLElement *element,
                                   const char *attribute_name,
                                   std::string &value) {
  if (element == NULL) {
    DLOG_ERROR(MOD_EB, "Load XMLAttribute Data error. xml_element is nullptr");
    return CFG_OP_INNER_ERR;
  }
  const tinyxml2::XMLAttribute *attr = element->FindAttribute(attribute_name);
  if (attr == NULL) {
    DLOG_WARNING(MOD_EB,
                 "Load XMLAttribute Data error. not have \"%s\"",
                 attribute_name);
    return CFG_OP_FILE_ERR;
  }
  value.clear();
  value.append(attr->Value());
  return CFG_OP_SUCCEED;
}

int ConfigServer::LoadXMLEleData(int paraIndex, config::ConfigData &data) {
  if (!xml_doc_.get()) {
    DLOG_ERROR(MOD_EB, "Load XMLDoc Data error.xml_doc_ is nullptr");
    return CFG_OP_INNER_ERR;
  }
  tinyxml2::XMLElement *root_element =
    xml_doc_->FirstChildElement(module_name_.c_str());
  if (!root_element) {
    DLOG_ERROR(MOD_EB, "XML file (group_root) format error.file:\"%s\".",
               file_path_.c_str());
    return CFG_OP_FILE_ERR;
  }
  char idx_str[100];
  sprintf(idx_str, "%d", paraIndex);
  tinyxml2::XMLElement *element = root_element->FirstChildElement();
  while (element != NULL) {
    const tinyxml2::XMLAttribute *idx_attr =
      element->FindAttribute(CFG_ATTR_NAME_ID);
    if (idx_attr) {
      const char *cur_idx = idx_attr->Value();
      if (strcmp(idx_str, cur_idx) == 0) {
        break;
      }
    } else {
      DLOG_ERROR(MOD_EB, "XML file (index) format error.file:\"%s\".",
                 file_path_.c_str());
      return CFG_OP_FILE_ERR;
    }
    element = element->NextSiblingElement();
  }
  if (element == NULL) {
    return CFG_OP_NOT_FOUND;
  }

  return LoadXMLEleData(element, data);
}

int ConfigServer::LoadXMLEleData(const std::string &param_name,
                                 config::ConfigData &data) {
  if (!xml_doc_.get()) {
    DLOG_ERROR(MOD_EB, "Load XMLDoc Data error.xml_doc_ is nullptr");
    return CFG_OP_INNER_ERR;
  }
  tinyxml2::XMLElement *root_element =
    xml_doc_->FirstChildElement(module_name_.c_str());
  if (!root_element) {
    DLOG_ERROR(MOD_EB, "XML file (group_root) format error.file:\"%s\".",
               file_path_.c_str());
    return CFG_OP_FILE_ERR;
  }
  tinyxml2::XMLElement *element =
    root_element->FirstChildElement(param_name.c_str());

  if (element == NULL) {
    return CFG_OP_NOT_FOUND;
  }
  return LoadXMLEleData(element, data);
}

int ConfigServer::LoadXMLEleData(tinyxml2::XMLElement *element,
                                 config::ConfigData &data) {
  if (element == NULL) {
    DLOG_ERROR(MOD_EB, "XML element param is null.");
    return CFG_OP_INNER_ERR;
  }

  data = ConfigData();
  int res;

  res = LoadXMLAttrValue(element, CFG_ATTR_NAME_MIN, data.min_value);
  if ( res != CFG_OP_SUCCEED) {
    return CFG_OP_FILE_ERR;
  }

  res = LoadXMLAttrValue(element, CFG_ATTR_NAME_MAX, data.max_value);
  if ( res != CFG_OP_SUCCEED) {
    return CFG_OP_FILE_ERR;
  }

  res = LoadXMLAttrValue(element, CFG_ATTR_NAME_DEFAULT, data.default_value);
  if ( res != CFG_OP_SUCCEED) {
    return CFG_OP_FILE_ERR;
  }

  std::string idx;
  res = LoadXMLAttrValue(element, CFG_ATTR_NAME_ID, idx);
  if ( res != CFG_OP_SUCCEED) {
    return CFG_OP_FILE_ERR;
  }
  if (sscanf(idx.c_str(), "%d", &data.index) != 1) {
    DLOG_ERROR(MOD_EB, "XML file (index) format error.file:\"%s\".",
               file_path_.c_str());
    return CFG_OP_FILE_ERR;
  }

  std::string para_type;
  res = LoadXMLAttrValue(element, CFG_ATTR_NAME_TYPE, para_type);
  if ( res != CFG_OP_SUCCEED) {
    return CFG_OP_FILE_ERR;
  }
  ConvertStringToConfigParamType(para_type, data.para_type);
  data.value.append(element->GetText());
  data.name = element->Name();
  data.element = element;

  return CFG_OP_SUCCEED;

}

int ConfigServer::GetCfgValByName(const std::string &paraName,
                                  std::string &val) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(paraName, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  return GetCfgVal(data, val);
}

int ConfigServer::GetCfgValById(int paraIndex, std::string &val) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(paraIndex, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  return GetCfgVal(data, val);
}

int ConfigServer::GetCfgVal(const ConfigData &data,
                            std::string &val,
                            bool use_default) {
  if (data.para_type != PARA_TYPE_STRING &&
      data.para_type != PARA_TYPE_USER_DEFINE) {
    return CFG_OP_TYPE_ERR;
  }
  if (data.value.length()) {
    val = data.value;
  } else {
    if (!use_default) {
      return CFG_OP_NO_VALUE;
    }
    val = data.default_value;
  }
  return CFG_OP_SUCCEED;
}

int ConfigServer::GetArrayCfgVal(const ConfigData &data,
                                 std::vector<ConfigParamNode> &configs) {
  tinyxml2::XMLElement *element;
  if (data.para_type != PARA_TYPE_ARRARY) {
    return CFG_OP_TYPE_ERR;
  }
  element = data.element->FirstChildElement(CFG_ATTR_NAME_ITEM);

  ConfigData item_data = data;
  std::string item_type_str;
  int ret = LoadXMLAttrValue(data.element, CFG_ATTR_NAME_ITEM_TYPE, item_type_str);
  if (ret != CFG_OP_SUCCEED) {
    return ret;
  }
  ConfigParamType item_type;
  ret = ConvertStringToConfigParamType(item_type_str, item_type);
  if (ret != CFG_OP_SUCCEED) {
    return ret;
  }
  item_data.para_type = item_type;

  while (element != NULL) {
    item_data.value.clear();
    item_data.value.append(element->GetText());
    ConfigParamNode config;
    int ret = GetCfgVal(item_data, config, false);
    if (ret != CFG_OP_SUCCEED && ret != CFG_OP_NO_VALUE) {
      return ret;
    }
    if (ret != CFG_OP_NO_VALUE) {
      configs.push_back(config);
    }
    element = element->NextSiblingElement();
  }
  return CFG_OP_SUCCEED;
}

int ConfigServer::ConvertStringToConfigParamType(const std::string &str,
    ConfigParamType &type) {
  int pt_i;
  if (sscanf(str.c_str(), "%d", &pt_i) != 1) {
    DLOG_ERROR(MOD_EB, "XML file (para_type:%s) format error.file:\"%s\".",
               str.c_str(), file_path_.c_str());
    return CFG_OP_FILE_ERR;
  } else if (pt_i < CFG_PARAM_TYPE_MIN_VAL || pt_i > CFG_PARAM_TYPE_MAX_VAL) {
    DLOG_ERROR(MOD_EB, "XML file (para_type:%s) format error.file:\"%s\".",
               str.c_str(), file_path_.c_str());
    return CFG_OP_FILE_ERR;
  }
  type = (ConfigParamType) pt_i;
  return CFG_OP_SUCCEED;
}

int ConfigServer::GetCfgVal(const ConfigData &data,
                            ConfigParamNode &config,
                            bool use_default) {
  config.index = data.index;
  config.key = data.name;
  config.type = data.para_type;
  if (data.para_type == PARA_TYPE_INT32) {
    config.val.resize(sizeof(int32));
    return GetCfgVal(data, (char *)config.val.c_str(), sizeof(int32), use_default);
  } else if (data.para_type == PARA_TYPE_FLOAT) {
    config.val.resize(sizeof(float));
    return GetCfgVal(data, (char *)config.val.c_str(), sizeof(float), use_default);
  } else if (data.para_type == PARA_TYPE_DOUBLE) {
    config.val.resize(sizeof(double));
    return GetCfgVal(data, (char *)config.val.c_str(), sizeof(double), use_default);
  } else if (data.para_type == PARA_TYPE_INT64) {
    config.val.resize(sizeof(int64));
    return GetCfgVal(data, (char *)config.val.c_str(), sizeof(int64), use_default);
  } else if (data.para_type == PARA_TYPE_STRING) {
    return GetCfgVal(data, config.val, use_default);
  } else if (data.para_type == PARA_TYPE_USER_DEFINE) {
    return GetCfgVal(data, config.val, use_default);
  }
  return CFG_OP_INNER_ERR;
}

int ConfigServer::GetCfgVal(const ConfigData &data,
                            void *val, int val_len,
                            bool use_default) {
  if (data.para_type == PARA_TYPE_INT32) {
    if (val_len != sizeof(int32)) {
      return CFG_OP_PARALEN_ERR;
    }
    if (!data.value.empty()) {
      int rd = sscanf(data.value.c_str(), "%d", (int *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    } else {
      if (!use_default) {
        return CFG_OP_NO_VALUE;
      }
      int rd = sscanf(data.default_value.c_str(), "%d", (int *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    }
  } else if (data.para_type == PARA_TYPE_DOUBLE) {
    if (val_len != sizeof(double)) {
      return CFG_OP_PARALEN_ERR;
    }
    if (!data.value.empty()) {
      int rd = sscanf(data.value.c_str(), "%lf", (double *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    } else {
      if (!use_default) {
        return CFG_OP_NO_VALUE;
      }
      int rd = sscanf(data.default_value.c_str(), "%lf", (double *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    }
  } else if (data.para_type == PARA_TYPE_FLOAT) {
    if (val_len != sizeof(float)) {
      return CFG_OP_PARALEN_ERR;
    }
    if (!data.value.empty()) {
      int rd = sscanf(data.value.c_str(), "%f", (float *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    } else {
      if (!use_default) {
        return CFG_OP_NO_VALUE;
      }
      int rd = sscanf(data.default_value.c_str(), "%f", (float *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    }
  } else if (data.para_type == PARA_TYPE_INT64) {
    if (val_len != sizeof(int64)) {
      return CFG_OP_PARALEN_ERR;
    }
    if (!data.value.empty()) {
      int rd = sscanf(data.value.c_str(), "%lld", (int64 *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    } else {
      if (!use_default) {
        return CFG_OP_NO_VALUE;
      }
      int rd = sscanf(data.default_value.c_str(), "%lld", (int64 *)val);
      if (rd != 1) {
        return CFG_OP_INNER_ERR;
      }
    }
  } else {
    return CFG_OP_INNER_ERR;
  }
  return CFG_OP_SUCCEED;
}


int ConfigServer::GetCfgValByName(const std::string &paraName,
                                  void *val,
                                  int val_len) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(paraName, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  return GetCfgVal(data, val, val_len);
}

int ConfigServer::GetCfgValById(int paraIndex,
                                void *val,
                                int val_len) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(paraIndex, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  return GetCfgVal(data, val, val_len);
}

int ConfigServer::SetCfgValByName(const std::string &param_name,
                                  const void *val, int val_len) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(param_name, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  if (data.para_type == PARA_TYPE_STRING) {
    return CFG_OP_TYPE_ERR;
  }
  std::string val_str;
  val_str.append((char *)val, val_len);
  ConfigParamNode config(param_name, data.para_type, val_str);
  return SetCfgVal(data, config);
}

int ConfigServer::SetCfgValByName(const std::string &param_name,
                                  const std::string &val) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(param_name, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  ConfigParamNode config(param_name, data.para_type, val);
  return SetCfgVal(data, config);
}

int ConfigServer::SetCfgValById(const int param_index,
                                const void *val, int val_len) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res = LoadXMLEleData(param_index, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  if (data.para_type == PARA_TYPE_STRING) {
    return CFG_OP_TYPE_ERR;
  }
  std::string val_str;
  val_str.append((char *)val, val_len);
  ConfigParamNode config(param_index, data.para_type, val_str);
  return SetCfgVal(data, config);
}

int ConfigServer::SetCfgValById(const int param_index,
                                const std::string &val) {
  vzes::CritScope cr(&crit_);
  if (!xml_doc_.get()) {
    return CFG_OP_FILE_ERR;
  }
  ConfigData data;
  int res = LoadXMLEleData(param_index, data);
  if (res != CFG_OP_SUCCEED) {
    return res;
  }
  ConfigParamNode config(param_index, data.para_type, val);
  return SetCfgVal(data, config);
}

int ConfigServer::SetArrayCfgVal(const ConfigData &data,
                                 const std::vector<ConfigParamNode> &configs) {
  if (configs.size() == 0) {
    return CFG_OP_INNER_ERR;
  }
  ConfigData item_data = data;
  std::string max_size_str;
  LoadXMLAttrValue(data.element, CFG_ATTR_NAME_ITEM_MAX_SIZE, max_size_str);
  int max_size;
  if (sscanf(max_size_str.c_str(), "%d", &max_size) != 1) {
    return CFG_OP_INNER_ERR;
  }
  if (max_size < configs.size()) {
    return CFG_OP_RANGE_ERR;
  }
  item_data.para_type = configs[0].type;
  data.element->DeleteChildren();
  for (int i = 0; i < configs.size(); i++) {
    tinyxml2::XMLElement *element = xml_doc_->NewElement(CFG_ATTR_NAME_ITEM);
    data.element->InsertEndChild(element);

    item_data.element = element;
    const ConfigParamNode &config = configs[i];

    int res = SetCfgVal(item_data, config, false);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
  }
  return CFG_OP_SUCCEED;
}

int ConfigServer::SetCfgVal(const ConfigData &data,
                            const ConfigParamNode &config,
                            bool write_to_file) {
  if (data.para_type != config.type) {
    return CFG_OP_TYPE_ERR;
  }
  if (data.para_type == PARA_TYPE_INT32) {
    if (config.val.size() != sizeof(int32)) {
      return CFG_OP_PARALEN_ERR;
    }
    int min_val, max_val, val;
    sscanf(data.min_value.c_str(), "%d", &min_val);
    sscanf(data.max_value.c_str(), "%d", &max_val);
    memcpy(&val, config.val.c_str(), config.val.size());
    if (min_val <= val && val <= max_val) {
      data.element->SetText(val);
    } else {
      return CFG_OP_RANGE_ERR;
    }
  } else if (data.para_type == PARA_TYPE_DOUBLE) {
    if (config.val.size() != sizeof(double)) {
      return CFG_OP_TYPE_ERR;
    }
    double min_val, max_val, val;
    sscanf(data.min_value.c_str(), "%lf", &min_val);
    sscanf(data.max_value.c_str(), "%lf", &max_val);
    memcpy(&val, config.val.c_str(), config.val.size());
    if (min_val - FLOAT_EPS <= val && val <= max_val + FLOAT_EPS) {
      data.element->SetText(val);
    } else {
      return CFG_OP_RANGE_ERR;
    }
  } else if (data.para_type == PARA_TYPE_FLOAT) {
    if (config.val.size() != sizeof(float)) {
      return CFG_OP_TYPE_ERR;
    }
    float min_val, max_val, val;
    sscanf(data.min_value.c_str(), "%f", &min_val);
    sscanf(data.max_value.c_str(), "%f", &max_val);
    memcpy(&val, config.val.c_str(), config.val.size());
    if (min_val - FLOAT_EPS <= val && val <= max_val + FLOAT_EPS) {
      data.element->SetText(val);
    } else {
      return CFG_OP_RANGE_ERR;
    }
  } else if (data.para_type == PARA_TYPE_INT64) {
    if (config.val.size() != sizeof(long long)) {
      return CFG_OP_TYPE_ERR;
    }
    int64 min_val, max_val, val;
    sscanf(data.min_value.c_str(), "%lld", &min_val);
    sscanf(data.max_value.c_str(), "%lld", &max_val);
    memcpy(&val, config.val.c_str(), config.val.size());
    if (min_val <= val && val <= max_val) {
      data.element->SetText(val);
    } else {
      return CFG_OP_RANGE_ERR;
    }
  } else if (data.para_type == PARA_TYPE_STRING) {
    int min_len, max_len;
    sscanf(data.min_value.c_str(), "%d", &min_len);
    sscanf(data.max_value.c_str(), "%d", &max_len);
    if (config.val.size() >= min_len && config.val.size() <= max_len) {
      data.element->SetText(config.val.c_str());
    } else {
      return CFG_OP_RANGE_ERR;
    }
  } else if (data.para_type == PARA_TYPE_USER_DEFINE) {
    data.element->SetText(config.val.c_str());
  } else {
    return CFG_OP_INNER_ERR;
  }
  if (write_to_file) {
    if (SaveXMLDoc()) {
      return CFG_OP_SUCCEED;
    } else {
      return CFG_OP_INNER_ERR;
    }
  } else {
    return CFG_OP_SUCCEED;
  }
}


int ConfigServer::SetCfgVal(const std::vector<config::ConfigParamNode> &configs,
                            bool use_call_cack) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  if (call_back_ && use_call_cack) {
    int ret = call_back_(module_name_, configs, call_back_user_data_);
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }

  for (int i = 0; i < configs.size(); i++) {
    const ConfigParamNode &config = configs[i];
    ConfigData data;

    int res;
    if (config.index) {
      res = LoadXMLEleData(config.index, data);
      if (res != CFG_OP_SUCCEED) {
        return res;
      }
    } else {
      res = LoadXMLEleData(config.key, data);
      if (res != CFG_OP_SUCCEED) {
        return res;
      }
    }
    if (data.para_type == PARA_TYPE_ARRARY) {
      std::vector<ConfigParamNode> configarr;
      for (int j = i; j < configs.size(); j++) {
        if (configs[j].index != config.index ||
            configs[j].key != config.key) {
          break;
        }
        configarr.push_back(configs[j]);
      }
      i += configarr.size() - 1;
      res = SetArrayCfgVal(data, configarr);
    } else {
      res = SetCfgVal(data, configs[i], false);
    }
    if (res != CFG_OP_SUCCEED) {
      xml_doc_.reset();
      DLOG_INFO(MOD_EB, "set configs error. clear xml_doc_ done.");
      return res;
    }
  }//for loop end
  if (SaveXMLDoc()) {
    return CFG_OP_SUCCEED;
  } else {
    return CFG_OP_INNER_ERR;
  }
}

int ConfigServer::GetAllCfgVal(std::vector<ConfigParamNode> &configs) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  tinyxml2::XMLElement *root_element =
    xml_doc_->FirstChildElement(module_name_.c_str());
  if (!root_element) {
    DLOG_ERROR(MOD_EB, "XML file (group_root) format error.file:\"%s\".",
               file_path_.c_str());
    return CFG_OP_FILE_ERR;
  }
  tinyxml2::XMLElement *element = root_element->FirstChildElement();
  while (element != NULL) {
    ConfigParamNode config;
    ConfigData data;
    int res;
    res = LoadXMLEleData(element, data);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
    if (data.para_type == PARA_TYPE_ARRARY) {
      std::vector<ConfigParamNode> arr_config;
      res = GetArrayCfgVal(data, arr_config);
      if (res != CFG_OP_SUCCEED) {
        return res;
      }
      for (int i = 0; i < arr_config.size(); i++) {
        configs.push_back(arr_config[i]);
      }
    } else {
      res = GetCfgVal(data, config);
      if (res != CFG_OP_SUCCEED) {
        return res;
      }
      configs.push_back(config);
    }
    element = element->NextSiblingElement();
  }

  return CFG_OP_SUCCEED;
}

int ConfigServer::GetArrayCfgVal(std::vector<ConfigParamNode> &configs) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }

  if (configs.size() != 1) {
    return CFG_OP_PARALEN_ERR;
  }
  ConfigParamNode config = configs[0];
  configs.clear();
  ConfigData data;
  int res;
  if (config.index) {
    res = LoadXMLEleData(config.index, data);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
  } else {
    res = LoadXMLEleData(config.key, data);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
  }
  return GetArrayCfgVal(data, configs);
}

int ConfigServer::GetCfgVal(ConfigParamNode &config) {
  vzes::CritScope cr(&crit_);
  if (!Inited()) {
    int ret = InitXMLDoc();
    if (ret != CFG_OP_SUCCEED) {
      return ret;
    }
  }
  ConfigData data;
  int res;
  if (config.index) {
    res = LoadXMLEleData(config.index, data);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
  } else {
    res = LoadXMLEleData(config.key, data);
    if (res != CFG_OP_SUCCEED) {
      return res;
    }
  }
  return GetCfgVal(data, config);
}

/*-----------------------------ConfigServerManager----------------------------*/
ConfigServerManager *cfgSrvManIns_ = NULL;

ConfigServerManager *ConfigServerManager::GetCfgSrvManIns() {
  if (cfgSrvManIns_ == NULL) {
    cfgSrvManIns_ = new ConfigServerManager();
    DLOG_INFO(MOD_EB, "Init ConfigServerManager  instance");
  }
  return cfgSrvManIns_;
}

ConfigServer::Ptr ConfigServerManager::GetConfigServer(
  std::string module_name,
  CFG_MODULE_PROPERTY module_property) {
  vzes::CritScope cr(&crit_);
  ConfigServerPtrs::iterator iter, tmp_iter;
  for (iter = config_servers_.begin(); iter != config_servers_.end(); iter++) {
    if (!iter->get()) {
      DLOG_ERROR(MOD_EB, "config_servers_ error ptr is nullptr");
      return ConfigServer::Ptr();
    }
    if ((*iter)->GetModuleName() != module_name) {
      continue;
    }
    if ((*iter)->GetModuleProperty() != module_property) {
      continue;
    }
    //find config_server;
    ConfigServer::Ptr ret = *iter;
    config_servers_.push_back(*iter);
    config_servers_.erase(iter);
    return ret;
  }
  if (CONFIG_CACHE_SIZE < 0) {
    DLOG_ERROR(MOD_EB, "CONFIG_CACHE_SIZE = %d. not allowed.");
  }
  while (config_servers_.size() >= CONFIG_CACHE_SIZE) {
    iter = config_servers_.begin();
    (*iter)->ResetXMLDoc();
    config_servers_.erase(iter);
  }

  ConfigServer::Ptr cfg_srv(new ConfigServer(module_name, module_property));
  if (!cfg_srv->Init()) {
    return ConfigServer::Ptr();
  }
  config_servers_.push_back(cfg_srv);
  return cfg_srv;
}

} //namespace config
