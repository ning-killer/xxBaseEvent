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

#include <astl/include/string.hpp>
#include "tinyxml2/tinyxml2.h"
#include "config/config/configclient.h"
#include <stdio.h>
#include <math.h>
#include "app/app/app.h"
#include "filecache/server/cachenetserver.h"

using namespace tinyxml2;
using namespace config;
const char *filepath = "D:\\config_db\\kvdb\\test.xml";
int global_count = 0;
int64 test_tot = 1e8;
#define VAULE_TYPE_DEFAULT 233
#define STRING_TYPE_DEFAULT "233"


void write_test() {
  XMLDocument doc;
  doc.InsertEndChild(doc.NewElement("int32"));
  doc.InsertEndChild(doc.NewElement("double"));
  doc.InsertEndChild(doc.NewElement("string"));
  doc.InsertEndChild(doc.NewElement("int64"));
  doc.FirstChildElement("double")->SetText(10.8);
  doc.FirstChildElement("string")->SetText("&lt;&gt;\"");
  doc.FirstChildElement("int64")->SetText(999999999999);
  doc.FirstChildElement("int64")->SetAttribute("attr_i", 188);
  doc.FirstChildElement("int64")->SetAttribute("attr_f", "&lt;&quot;&gt;\"");
  doc.FirstChildElement("int64")->SetAttribute("attr_s", "23");
  XMLPrinter printer;
  doc.Print(&printer);
  printf("%s", printer.CStr());
  doc.SaveFile(filepath);
}

void read_test() {
  XMLDocument doc;
  int ret = doc.LoadFile(filepath);

  XMLElement *first = doc.FirstChildElement();
  while (first) {
    printf("fi:%s\n", first->Name());
    if (true) {
      const char *p = first->GetText();
      printf("se:%s\n", p);
    }
    first = first->NextSiblingElement();
  }
  XMLPrinter printer;
  doc.Print(&printer);
  printf("\"%s\"", printer.CStr());
  char *p = (char *) printer.CStr();

  return;
}

int user_check(std::string groupName,
               std::vector<ConfigParamNode> params,
               void *userData) {
  if (groupName != "test") {
    return CFG_OP_TYPE_ERR;
  }
  int64 sum = 0;
  std::vector<ConfigParamNode>::iterator it;
  for (it = params.begin(); it != params.end(); it++) {
    ConfigParamNode cfg = *it;
    if (cfg.type != PARA_TYPE_INT64) {
      return CFG_OP_TYPE_ERR;
    }
    sum += *(int64 *) cfg.val.c_str();
  }
  if (sum == *(int64 *) userData) {
    return CFG_OP_SUCCEED;
  } else {
    return CFG_OP_TYPE_ERR;
  }
}

class ConfigClientApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<
  ConfigClientApp>,
  public sigslot::has_slots<> {
 public:
  ConfigClientApp(std::string str,
                  CFG_MODULE_PROPERTY pgp = CFG_M_SEC_NOR)
    : AppInterface("ConfigClientApp") {
    name_ = str;
    pgp_ = pgp;
  }
  virtual ~ConfigClientApp() {

  }
  virtual bool ConfigClientApp::RunAPP(vzes::EventService::Ptr event_service);

  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    Log_DbgSetLevel(MOD_EB, LL_DEBUG);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }


  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
  bool int_test();
  bool double_test();
  bool float_test();
  bool int64_test();
  bool string_test();
  bool default_test();
  bool error_type_test();
  bool call_back_test();
  bool configs_node_test();
  std::string name_;
  CFG_MODULE_PROPERTY pgp_;
};

int main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr configClientApp0(new ConfigClientApp("test"));
  app::AppInterface::Ptr configClientApp1(new ConfigClientApp("test"));
  app::AppInterface::Ptr
  configClientApp2(new ConfigClientApp("test", CFG_M_SEC_ONE));
  app::AppInterface::Ptr
  configClientApp3(new ConfigClientApp("test", CFG_M_SEC_TWO));
  app::AppInterface::Ptr
  configClientApp4(new ConfigClientApp("test", CFG_M_SEC_THREE));

  app->RegisterApp(configClientApp0);
  /*
  app->RegisterApp(configClientApp1);
  app->RegisterApp(configClientApp2);
  app->RegisterApp(configClientApp3);
  app->RegisterApp(configClientApp4);
  */
  app->AppRun();
  while (1);
}

bool ConfigClientApp::configs_node_test() {
  DLOG_KEY(MOD_EB, "configs_node_test");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  cfg_client->SetPreCheckValFunc(NULL, NULL, name_, pgp_);
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    std::vector<config::ConfigParamNode> configs,cfg_v;
    cfg_client->GetAllCfgVal(configs, name_, pgp_);
    if (configs.size() != 7) {
      succeed = false;
    }
    for (int i = 0; i < 6; i++) {
      config::ConfigParamNode &cfg = configs[i];
      if (cfg_client->SetCfgVal(cfg, name_, pgp_) != CFG_OP_SUCCEED) {
        succeed = false;
      }
    }
    configs.push_back(configs[5]);
    configs.push_back(configs[6]);
    if (cfg_client->SetCfgVal(configs, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    cfg_client->GetAllCfgVal(cfg_v, name_, pgp_);
    if (configs.size() != 9) {
      succeed = false;
    }
    configs.push_back(configs[5]);
    if (cfg_client->SetCfgVal(configs, name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    cfg_client->GetAllCfgVal(cfg_v, name_, pgp_);
    if (cfg_v.size() != 9) {
      succeed = false;
    }
  }
  return succeed;
}

bool ConfigClientApp::RunAPP(vzes::EventService::Ptr event_service) {
  while (1) {
    if (default_test()) {
      DLOG_KEY(MOD_EB, "default test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "default test failed");
    }
    if (error_type_test()) {
      DLOG_KEY(MOD_EB, "error_type test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "error_type test failed");
    }
    if (call_back_test()) {
      DLOG_KEY(MOD_EB, "call_back test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "call_back test failed");
    }
    if (int_test()) {
      DLOG_KEY(MOD_EB, "int test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "int test failed");
    }

    if (double_test()) {
      DLOG_KEY(MOD_EB, "double test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "double test failed");
    }

    if (float_test()) {
      DLOG_KEY(MOD_EB, "float test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "float test failed");
    }

    if (int64_test()) {
      DLOG_KEY(MOD_EB, "int64 test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "int64 test failed");
    }

    if (string_test()) {
      DLOG_KEY(MOD_EB, "string test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "string test failed");
    }

    if (configs_node_test()) {
      DLOG_KEY(MOD_EB, "configs_node_test succeed");
    } else {
      DLOG_ERROR(MOD_EB, "configs_node_test failed");
    }
    vzsleep(10000000);
  }
  return true;
}

bool ConfigClientApp::int_test() {
  DLOG_KEY(MOD_EB, "int_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    int val = 5 + global_count++;
    int rd;
    //name
    if (cfg_client->SetCfgValByName("int", &val, sizeof(int), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("int", &rd, sizeof(int), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }
    //id
    val = 5 + global_count++;
    if (cfg_client->SetCfgValById(1, &val, sizeof(int), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValById(1, &rd, sizeof(int), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }

    //error_range
    rd = -1100000;
    if (cfg_client->SetCfgValById(1, &rd, sizeof(int), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    rd = 1100000;
    if (cfg_client->SetCfgValByName("int", &rd, sizeof(int), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    //检验错误修改后值是否变化
    if (cfg_client->GetCfgValByName("int", &rd, sizeof(int), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }

  }
  return succeed;
}

bool ConfigClientApp::double_test() {
  DLOG_KEY(MOD_EB, "double_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    double val = 5.5 + global_count++;
    double rd;
    //name
    if (cfg_client->SetCfgValByName("double", &val, sizeof(double), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("double", &rd, sizeof(double), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }
    //id
    val = 5.5 + global_count++;
    if (cfg_client->SetCfgValById(2, &val, sizeof(double), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValById(2, &rd, sizeof(double), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }

    //error_range
    rd = -1e9;
    if (cfg_client->SetCfgValById(2, &rd, sizeof(double), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    rd = 1100000000;
    if (cfg_client->SetCfgValByName("double", &rd, sizeof(double), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    //检验错误修改后值是否变化
    if (cfg_client->GetCfgValByName("double", &rd, sizeof(double), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }

  }
  return succeed;
}

bool ConfigClientApp::float_test() {
  DLOG_KEY(MOD_EB, "float_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    float val = 5.5 + global_count++;
    float rd;
    //name
    if (cfg_client->SetCfgValByName("float", &val, sizeof(float), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("float", &rd, sizeof(float), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }
    //id
    val = 5.5 + global_count++;
    if (cfg_client->SetCfgValById(3, &val, sizeof(float), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValById(3, &rd, sizeof(float), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }

    //error_range
    rd = -1e8;
    if (cfg_client->SetCfgValById(3, &rd, sizeof(float), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    rd = 1e10;
    if (cfg_client->SetCfgValByName("float", &rd, sizeof(float), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    //检验错误修改后值是否变化
    if (cfg_client->GetCfgValByName("float", &rd, sizeof(float), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(val - rd) > 1e-9) {
      succeed = false;
    }

  }
  return succeed;
}

bool ConfigClientApp::int64_test() {
  DLOG_KEY(MOD_EB, "int64_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    int64 val = 1e16 + global_count++;
    int64 rd;
    //name
    if (cfg_client->SetCfgValByName("int64", &val, sizeof(int64), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("int64", &rd, sizeof(int64), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }
    //id
    val = 5 + global_count++;
    if (cfg_client->SetCfgValById(4, &val, sizeof(int64), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValById(4, &rd, sizeof(int64), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }

    //error_range
    rd = -1e10;
    if (cfg_client->SetCfgValById(4, &rd, sizeof(int64), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    rd = 5e18;
    if (cfg_client->SetCfgValByName("int64", &rd, sizeof(int64), name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    //检验错误修改后值是否变化
    if (cfg_client->GetCfgValByName("int64", &rd, sizeof(int64), name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != val) {
      succeed = false;
    }

  }
  return succeed;
}

bool ConfigClientApp::string_test() {
  DLOG_KEY(MOD_EB, "string_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    char buffer[1000];
    sprintf(buffer, "test_test%d", global_count++);
    std::string str(buffer);
    std::string rd;
    //name
    if (cfg_client->SetCfgValByName("string", str, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("string", rd, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != str) {
      succeed = false;
    }
    //id
    sprintf(buffer, "test_test%d", global_count++);
    str = std::string(buffer);
    if (cfg_client->SetCfgValById(5, str, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->GetCfgValById(5, rd, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != str) {
      succeed = false;
    }

    //error_range
    rd = "1";
    if (cfg_client->SetCfgValById(5, rd, name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    rd = "321312312312312312312312312313123412341234123413413241234123413l";
    rd.append("1231231341834y1kjmfnsvmxcznvksdnfklnzlxmcnvklnaedf213h4");
    if (cfg_client->SetCfgValById(5, rd, name_, pgp_) != CFG_OP_RANGE_ERR) {
      succeed = false;
    }
    //检验错误修改后值是否变化
    if (cfg_client->GetCfgValByName("string", rd, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (rd != str) {
      succeed = false;
    }

  }
  return succeed;
}

bool ConfigClientApp::call_back_test() {
  DLOG_KEY(MOD_EB, "call_back_test start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    int64 num1 = 4e7;
    int64 num2 = test_tot - num1;
    std::vector<ConfigParamNode> v, v2;
    std::string num1_str, num2_str;
    num1_str.resize(sizeof(int64));
    memcpy((void *)num1_str.c_str(), &num1, sizeof(int64));
    num2_str.resize(sizeof(int64));
    memcpy((void *)num2_str.c_str(), &num2, sizeof(int64));
    ConfigParamNode config("int64", PARA_TYPE_INT64, num1_str);
    ConfigParamNode config2("int64", PARA_TYPE_INT64, num2_str);
    v.push_back(config);
    v.push_back(config2);
    v2.push_back(config2);
    v2.push_back(config2);
    int ret;
    cfg_client->SetPreCheckValFunc(NULL, NULL, name_, pgp_);
    if (ret = cfg_client->SetCfgVal(v2, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    cfg_client->SetPreCheckValFunc(user_check, &test_tot, name_, pgp_);
    if (ret = cfg_client->SetCfgVal(v, name_, pgp_) != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (ret = cfg_client->SetCfgVal(v2, name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
  }

  return succeed;
}

bool ConfigClientApp::default_test() {
  DLOG_KEY(MOD_EB, "default start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    int vi;
    double vd;
    float vf;
    int64 vi64;
    std::string str;
    if (cfg_client->GetCfgValByName("int", &vi, sizeof(int), name_, pgp_)
        != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (vi != VAULE_TYPE_DEFAULT) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("double", &vd, sizeof(double), name_, pgp_)
        != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(VAULE_TYPE_DEFAULT - vd) > 1e-9) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("float", &vf, sizeof(float), name_, pgp_)
        != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (abs(VAULE_TYPE_DEFAULT - vf) > 1e-9) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("int64", &vi64, sizeof(int64), name_, pgp_)
        != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (vi64 != (int64)VAULE_TYPE_DEFAULT *1000000000) {
      succeed = false;
    }
    if (cfg_client->GetCfgValByName("string", str, name_, pgp_)
        != CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (str != STRING_TYPE_DEFAULT) {
      succeed = false;
    }
  }
  return succeed;

}

bool ConfigClientApp::error_type_test() {
  DLOG_KEY(MOD_EB, "error_type start");
  bool succeed = true;
  ConfigClient::Ptr
  cfg_client = ConfigClient::CreateConfigClient();
  if (!cfg_client.get()) {
    succeed = false;
  } else {
    int vi = 1;
    double vd = 1;
    float vf = 1;
    int64 vi64 = 1;
    std::string str = "1";
    if (cfg_client->SetCfgValByName("int", str, name_, pgp_)
        == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("int", &vf, sizeof(float), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("double", &vi, sizeof(int), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("double", &vf, sizeof(float), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("float", &vd, sizeof(double), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("float", &vi64, sizeof(int64), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("int64", &vi, sizeof(int), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("int64", &vd, sizeof(double), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("string", &vi, sizeof(int), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("string", &vd, sizeof(double), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
    if (cfg_client->SetCfgValByName("string", &vi64, sizeof(int64), name_, pgp_) == CFG_OP_SUCCEED) {
      succeed = false;
    }
  }
  return succeed;

}

