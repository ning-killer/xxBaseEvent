#include <iostream>
#include <stdio.h>
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include "eventservice/dp/dpserver.h"
#include "eventservice/base/basicincludes.h"

#define DP_MSG_HELLO     "HELLO"
#define DP_MSG_WORLD     "WORLD"
#define DP_MSG_TIMEOUT   "TIME-OUT"
#define DP_MESSAGE_DATA  "DP_MESSAGE_DATA: 1234567890"

class DPClient : public app::AppInterface,
  public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<DPClient>,
  public sigslot::has_slots<> {
 public:
  DPClient() :AppInterface("DPClient") {
  }
  virtual ~DPClient() {
  }

  bool Init() {
    vzes::SocketAddress address("192.168.6.244", 5293);
    dp_client_ = vzes::DpClient::CreateDpNetClient(event_service_, address);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(this, &DPClient::OnDpMessage);
    dp_client_->SignalErrorEvent.connect(this, &DPClient::OnErrorMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);
    dp_client_->ListenMessage(DP_MSG_TIMEOUT);
    return true;
  }

  void TestCase_1() {
    // DP Message test case
    printf("\n\n");
    DLOG_INFO(MOD_EB, ">>>> test case <1> start: DP Message test <<<<");
    if (NULL == dp_client_.get()) {
      if (!Init()) {
        return;
      }
    }

    bool ret = dp_client_->SendDpMessage(DP_MSG_HELLO, 0, DP_MESSAGE_DATA,
                                         strlen(DP_MESSAGE_DATA));
    if (ret) {
      DLOG_INFO(MOD_EB, "Send dp message \"%s\" successed", DP_MSG_HELLO);
    } else {
      DLOG_INFO(MOD_EB, "Send dp message \"%s\" failed", DP_MSG_HELLO);
    }
  }

  void TestCase_2() {
    // Request test case
    printf("\n\n");
    DLOG_INFO(MOD_EB, ">>>> test case <2> start: Request test <<<<");
    if (NULL == dp_client_.get()) {
      return;
    }
    vzes::DpBuffer res_buffer;
    bool ret = dp_client_->SendDpRequest(DP_MSG_HELLO, 1, NULL, 0, &res_buffer, 5*1000);
    if (ret) {
      DLOG_INFO(MOD_EB, "Send dp request \"%s\" successed, response:%s",
                DP_MSG_HELLO, res_buffer.c_str());
    } else {
      DLOG_INFO(MOD_EB, "Send dp request \"%s\" failed", DP_MSG_HELLO);
    }
  }

  void TestCase_3() {
    // Request timeout test case
    printf("\n\n");
    DLOG_INFO(MOD_EB, ">>>> test case <3> start: Request timeout test <<<<");
    if (NULL == dp_client_.get()) {
      return;
    }
    vzes::DpBuffer res_buffer;
    bool ret = dp_client_->SendDpRequest(DP_MSG_TIMEOUT, 1, NULL, 0,
                                         &res_buffer, 5000);
    if (ret) {
      DLOG_INFO(MOD_EB, "Send dp request \"%s\" successed, response:%s",
                DP_MSG_TIMEOUT, res_buffer.c_str());
    } else {
      DLOG_INFO(MOD_EB, "Send dp request \"%s\"failed", DP_MSG_TIMEOUT);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    return Init();
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    event_service_->PostDelayed(3*1000, this);
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  virtual void OnMessage(vzes::Message *msg) {
    static uint32 count = 0;
    count++;
    switch (count) {
    case 1: {
      TestCase_1();
      break;
    }
    case 2: {
      TestCase_2();
      break;
    }
    case 3: {
      TestCase_3();
      break;
    }
    default: {
      count = 0;
      break;
    }
    }

    event_service_->PostDelayed(5*1000, this, 0);
  }

  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    switch (dp_msg->type) {
    case vzes::TYPE_MESSAGE: {
      DLOG_INFO(MOD_EB, "Received dp message:%s, data:%s",
                dp_msg->method.c_str(),
                dp_msg->req_buffer.c_str());
      break;
    }

    case vzes::TYPE_REQUEST: {
      if (dp_msg->method == DP_MSG_WORLD) {
        // Replay DP request
        DLOG_INFO(MOD_EB, "Received DP request %s", dp_msg->method.c_str());
        dp_msg->res_buffer->append("yyyyyyyyyyyyyy");
        bool ret = dp_client_->SendDpReply(dp_msg);
        if (ret) {
          DLOG_INFO(MOD_EB, "Replay message \"%s\" successed", dp_msg->method.c_str());
        } else {
          DLOG_ERROR(MOD_EB, "Replay message \"%s\" failed", dp_msg->method.c_str());
        }
      } else if (dp_msg->method == DP_MSG_TIMEOUT) {
        vzsleep(500);
        // Replay DP request
        DLOG_INFO(MOD_EB, "Received DP request %s", dp_msg->method.c_str());
        dp_msg->res_buffer->append("wwwwwwwwwwwwww");
        bool ret = dp_client_->SendDpReply(dp_msg);
        if (ret) {
          DLOG_INFO(MOD_EB, "Replay message \"%s\" successed", dp_msg->method.c_str());
        } else {
          DLOG_ERROR(MOD_EB, "Replay message \"%s\" failed", dp_msg->method.c_str());
        }
      }
      break;
    }

    default:
      break;
    }
  }

  void OnErrorMessage(vzes::DpClient::Ptr dp_client, int err) {
    switch (err) {
    case vzes::TYPE_ERROR_DISCONNECTED: {
      DLOG_ERROR(MOD_EB, "Error: %d", err);
      dp_client_.reset();
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
  app::AppInterface::Ptr dpnet_client(new DPClient());
  app->RegisterApp(dpnet_client);
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}
