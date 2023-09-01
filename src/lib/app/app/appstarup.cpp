#include "app/app/appstarup.h"

#include <iostream>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "filecache/kvdb/kvdbclient.h"
#include "filecache/cache/cacheclient.h"
#include "log/log/log_client.h"
#include <stdio.h>

#define DP_MSG_HELLO  "HELLO"
#define DP_MSG_WORLD  "WORLD"

#define DP_MESSAGE_DATA "DP_MESSAGE_DATA"

#ifdef WIN32
#define FC_FILE_PATH    "E:/fielcache_test.txt"
#else
#define FC_FILE_PATH    "/tmp/app/exec/fielcache_test.txt"
#endif

char file_content[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";


class EchoClient : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoClient>,
  public sigslot::has_slots<> {
 public:
  EchoClient() :AppInterface("EchoClient") {
  }
  virtual ~EchoClient() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    dp_client_     = vzes::DpClient::CreateDpClient(event_service_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(
      this, &EchoClient::OnDpMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);

    kvdb_client_  = cache::KvdbClient::CreateKvdbClient("tkvdb");
    cache_client_ = cache::CacheClient::CreateCacheClient();
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    DLOG_INFO(MOD_EB, "Send dp message");
    dp_client_->SendDpMessage(DP_MSG_HELLO,
                              0,
                              DP_MESSAGE_DATA,
                              strlen(DP_MESSAGE_DATA));
    DLOG_INFO(MOD_EB, "Send dp request start");
    std::string res;//*res = NULL;
    dp_client_->SendDpRequest(DP_MSG_HELLO,
                              0,
                              DP_MESSAGE_DATA,
                              strlen(DP_MESSAGE_DATA),
                              &res,
                              10000);
    DLOG_INFO(MOD_EB, "Send dp request done");
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    DLOG_INFO(MOD_EB, "ECHO CLIENT OnDpMessage");
  }
 private:
  vzes::EventService::Ptr event_service_;
  vzes::DpClient::Ptr     dp_client_;
  cache::KvdbClient::Ptr  kvdb_client_;
  cache::CacheClient::Ptr cache_client_;
};

class EchoServer : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoServer>,
  public sigslot::has_slots<> {
 public:
  EchoServer() :AppInterface("EchoServer") {
  }
  virtual ~EchoServer() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    dp_client_     = vzes::DpClient::CreateDpClient(event_service_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(
      this, &EchoServer::OnDpMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    if (dp_msg->type == vzes::TYPE_MESSAGE) {
      DLOG_INFO(MOD_EB, "ECHO SERVER Receive dp message");
      dp_client_->SendDpMessage(DP_MSG_HELLO,
                                0,
                                DP_MESSAGE_DATA,
                                strlen(DP_MESSAGE_DATA));
    } else if (dp_msg->type == vzes::TYPE_REQUEST) {
      DLOG_INFO(MOD_EB, "ECHO SERVER Recive dp request");
      dp_client_->SendDpReply(dp_msg);
    }
  }
 private:
  vzes::EventService::Ptr event_service_;
  vzes::DpClient::Ptr     dp_client_;
};

int vzes_app_main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr echo_client(new EchoClient());
  app::AppInterface::Ptr echo_server(new EchoServer());
  app->RegisterApp(echo_client);
  app->RegisterApp(echo_server);
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}