#include <iostream>
#include <stdio.h>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpserver.h"
#include "eventservice/base/basicincludes.h"

#define DP_MSG_HELLO     "HELLO"
#define DP_MSG_WORLD     "WORLD"
#define DP_MSG_TIMEOUT   "TIME-OUT"
#define DP_MESSAGE_DATA  "DP_MESSAGE_DATAssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"

class DPServer : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<DPServer>,
  public sigslot::has_slots<> {
 public:
  DPServer() :AppInterface("DPServer") {
  }
  virtual ~DPServer() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    vzes::SocketAddress address("0.0.0.0", 5293);
    bool res = vzes::InitDpNetServer(address);
    if (!res) {
      DLOG_ERROR(MOD_EB, "Init DP Net Server falied");
      vzsleep(5*1000);
      return false;
    }
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    dp_client_ = vzes::DpClient::CreateDpClient(event_service_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(
      this, &DPServer::OnDpMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);
    dp_client_->ListenMessage(DP_MSG_TIMEOUT);
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  virtual void OnMessage(vzes::Message *msg) {
  }

  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    switch (dp_msg->type) {
    case vzes::TYPE_MESSAGE: {
      // 1.Send DP message test
      DLOG_INFO(MOD_EB, "Received dp message:%s, data:%s",
                dp_msg->method.c_str(),
                dp_msg->req_buffer.c_str());

      bool ret = dp_client_->SendDpMessage(DP_MSG_WORLD, 0, DP_MESSAGE_DATA,
                                           strlen(DP_MESSAGE_DATA));
      if (ret) {
        DLOG_INFO(MOD_EB, "Send dp message \"%s\" successed", DP_MSG_WORLD);
      } else {
        DLOG_INFO(MOD_EB, "Send dp message \"%s\" failed", DP_MSG_WORLD);
      }
      break;
    }

    case vzes::TYPE_REQUEST: {
      if (dp_msg->method == DP_MSG_HELLO) {
        // 2.Replay DP request test
        DLOG_INFO(MOD_EB, "Received DP request %s", dp_msg->method.c_str());
        dp_msg->res_buffer->append("xxxxxxxxxxxxxxx");
        bool ret = dp_client_->SendDpReply(dp_msg);
        if (ret) {
          DLOG_INFO(MOD_EB, "Replay request \"%s\" successed", dp_msg->method.c_str());
        } else {
          DLOG_ERROR(MOD_EB, "Replay request \"%s\" failed", dp_msg->method.c_str());
        }

        vzsleep(1*1000);
        // 3.Send DP request test
        vzes::DpBuffer res_buffer;
        ret = dp_client_->SendDpRequest(DP_MSG_WORLD, 1, NULL, 0, &res_buffer, 5*1000);
        if (ret) {
          DLOG_INFO(MOD_EB, "Send dp request \"%s\" successed, response:%s",
                    DP_MSG_WORLD, res_buffer.c_str());
        } else {
          DLOG_ERROR(MOD_EB, "Send dp request \"%s\"failed", DP_MSG_WORLD);
        }
      } else if (dp_msg->method == DP_MSG_TIMEOUT) {
        // 4.Replay timeout test
        vzsleep(500);
        // Replay DP request
        DLOG_INFO(MOD_EB, "Received DP request %s", dp_msg->method.c_str());
        dp_msg->res_buffer->append("zzzzzzzzzzzzzzzzz");
        bool ret = dp_client_->SendDpReply(dp_msg);
        if (ret) {
          DLOG_INFO(MOD_EB, "Replay request \"%s\" successed", dp_msg->method.c_str());
        } else {
          DLOG_ERROR(MOD_EB, "Replay request \"%s\" failed", dp_msg->method.c_str());
        }

        // 5.Request timeout test
        vzsleep(500);
        // Send DP request
        vzes::DpBuffer res_buffer;
        ret = dp_client_->SendDpRequest(DP_MSG_TIMEOUT, 1, NULL, 0, &res_buffer, 5000);
        if (ret) {
          DLOG_INFO(MOD_EB, "Send dp request \"%s\" successed, response:%s",
                    DP_MSG_TIMEOUT, res_buffer.c_str());
        } else {
          DLOG_INFO(MOD_EB, "Send dp reques \"%s\" failed", DP_MSG_TIMEOUT);
        }
      }
      break;
    }

    default:
      break;
    }
  }

 private:
  vzes::EventService::Ptr event_service_;
  vzes::DpClient::Ptr     dp_client_;
};

int main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  Log_SetSyncPrint(true);
  app::AppInterface::Ptr dpnet_server(new DPServer());
  app->RegisterApp(dpnet_server);
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}
