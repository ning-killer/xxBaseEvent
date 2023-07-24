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

#include <iostream>
#include <stdio.h>
#include "eventservice/net/eventservice.h"
#include "log/log/log_client.h"

typedef struct FaceRecoInfoBase {
  unsigned int sequence;  //抓拍序号，从1开始，每产生一组抓拍数据增加1。
  char camId[32];         //相机编号
  char posId[32];         //点位编号
  char posName[96];       //点位名称

  unsigned int tvSec;     //抓拍时间秒数，从1970年01月01日00时00分00秒至抓拍时间经过的秒数。
  unsigned int tvUsec;    //抓拍时间微秒数，tvSec的尾数

  short isRealtimeData;   //实时抓拍标志，0：非实时抓拍数据。非0：实时抓拍数据。

  short matched;          //比对结果，0：未比对。-1：比对失败。大于0的取值：比对成功时的确信度分数（100分制）。
  char matchPersonId[20]; //人员ID
  char matchPersonName[16];//人员姓名	//utf-8
  int matchRole;          //人员角色，0：普通人员。 1：白名单人员。 2：黑名单人员

  int existImg;           //全景图，是否包含全景图像。0：不包含全景图像。非0：包含全景图像。
  char imgFormat[4];      //全景图像格式
  int imgLen;             //全景图像大小
  unsigned short faceXInImg;//人脸位于全景图像的X坐标。matched
  unsigned short faceYInImg;//人脸位于全景图像的y坐标
  unsigned short faceWInImg;//人脸位于全景图像宽度
  unsigned short faceHInImg;//人脸位于全景图像高度

  int existFaceImg;       //人脸图，特写图像标志，是否包含特写图像。0：不包含特写图像。非0：包含特写图像。
  char faceImgFormat[4];  //人脸图像封装格式。
  int faceImgLen;         //特写图像大小
  unsigned short faceXInFaceImg;//人脸位于特写图像的X坐标。
  unsigned short faceYInFaceImg;//人脸位于特写图像的y坐标。
  unsigned short faceWInFaceImg;//人脸位于特写图像的宽度
  unsigned short faceHInFaceImg;//人脸位于特写图像的高度

  int existVideo;         //是否包含视频。0：不包含视频。非0：包含视频。
  unsigned int videoStartSec;//视频起始时间（秒）
  unsigned int videoStartUsec;//videoStartSec尾数，微妙
  unsigned int videoEndSec;   //视频结束时间（秒）
  unsigned int videoEndUsec;  //videoEndSec尾数，微妙
  char videoFormat[4];        //视频封装格式。
  int videoLen;               //视频大小

  unsigned char sex;          //性别 0: 无此信息 1：男 2：女
  unsigned char age;          //年龄 0: 无此信息 其它值：年龄
  unsigned char expression;   //表情 0: 无此信息 其它值：暂未定义
  unsigned char skinColour;   //肤色 0: 无此信息 其它值：暂未定义
  unsigned char qValue;       //注册标准分数，分数越高越适合用来注册
  char resv[123];         //保留

  int feature_size;           //当前抓拍人脸的特征数据大小
  //float feature[kFeatureSize]; //当前抓拍人脸的特征数据
  int modelFaceImgLen;        //模板人脸图像长度
  char modelFaceImgFmt[4];       //模板人脸图像类型
} FaceRecoInfoBase;

const char NOT_FOUND[] =
  "HTTP/1.0 200 OK\r\n"
  "Server:Apache Tomcat/5.0.12\r\n"
  "Content-Type:text/html\r\n\r\n"
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "404 Not Found</h1></body>"
  "</html>";

class TcpServer : public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<TcpServer>,
  public sigslot::has_slots<> {
 public:
  TcpServer(vzes::EventService::Ptr event_service)
    : event_service_(event_service) {
  }
  bool Start() {
    ASSERT_RETURN_FAILURE(listenser_, false);
    listenser_ = event_service_->CreateAsyncListener();
    vzes::SocketAddress address("0.0.0.0", 8097);

    listenser_->SignalNewConnected.connect(
      this, &TcpServer::OnLisenerAcceptEvent);

    return listenser_->Start(address, false);
  }
 private:
  void OnLisenerAcceptEvent(vzes::AsyncListener::Ptr listener,
                            vzes::Socket::Ptr s,
                            int err) {

    vzes::AsyncSocket::Ptr async_socket = event_service_->CreateAsyncSocket(s);

    DLOG_INFO(MOD_EB, "Accept remote socket");
    if (async_socket && async_socket->IsConnected()) {
      //
      async_socket->SignalSocketWriteEvent.connect(
        this, &TcpServer::OnSocketWriteComplete);
      async_socket->SignalSocketReadEvent.connect(
        this, &TcpServer::OnSocketReadComplete);
      async_socket->SignalSocketErrorEvent.connect(
        this, &TcpServer::OnSocketErrorEvent);
      async_socket->AsyncRead();
      sockets_.push_back(async_socket);
      //if (sockets_.size() == 1) {
      //  event_service_->PostDelayed(5*1000, this, 0);
      //}
    }
  }

  void OnLisenerErrorEvent(vzes::AsyncListener::Ptr listener, int err) {
    DLOG_INFO(MOD_EB, "error event received");
  }

  ///
  void OnSocketWriteComplete(vzes::AsyncSocket::Ptr async_socket) {
    //DLOG_INFO(MOD_EB, "send data done");
  }

  void OnSocketReadComplete(vzes::AsyncSocket::Ptr async_socket,
                            vzes::MemBuffer::Ptr data) {
    //DLOG_INFO(MOD_EB, "received data, size = %d", data->size());
    //vzes::BlocksPtr &blocks = data->blocks();
    //for (vzes::BlocksPtr::iterator iter = blocks.begin();
    //     iter != blocks.end(); iter++) {
    //  vzes::Block::Ptr block = *iter;
    //  LOG(L_INFO).write((const char*)block->buffer, block->buffer_size);
    //}

    async_socket->AsyncWrite(NOT_FOUND, strlen(NOT_FOUND));
    async_socket->AsyncRead();
  }
  //  if (data->size() > 1024) {
  //    FaceRecoInfoBase frib;
  //    char *m_tcp_buf_ = (char*)malloc(1024 * 1024);
  //    data->CopyBytes((char*)&frib, 0, sizeof(FaceRecoInfoBase));
  //    async_socket->AsyncRead();
  //    return;
  //  }
  //  async_socket->AsyncRead();
  //}

  void OnSocketErrorEvent(vzes::AsyncSocket::Ptr async_socket,
                          int err) {
    async_socket->Close();
    sockets_.remove(async_socket);
  }

  virtual void OnMessage(vzes::Message *msg) {
    uint32 count = pack_cnt_;
    uint32 size = pack_size_;
    pack_cnt_ = 0;
    pack_size_ = 0;
    DLOG_INFO(MOD_EB, "received packets count = %d, total size = %d"
              ", speed = %d(kbps)", count, size, (8*size)/1024/5);
    //vzes::EventService::DumpAsyncSocketMemInfo();
    //event_service_->PostDelayed(5*1000, this, 0);
  }
 private:
  uint32                        pack_cnt_;
  uint64                        pack_size_;
  vzes::EventService::Ptr       event_service_;
  typedef std::list<vzes::AsyncSocket::Ptr> ASSockets;
  vzes::AsyncListener::Ptr      listenser_;
  ASSockets                     sockets_;
};

int main(void) {
  (void)Log_Init(false);
  DLOG_INFO(MOD_EB, "Start Create EventService");
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  DLOG_INFO(MOD_EB, "Start Create EventService");
  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("TcpServer");
  DLOG_INFO(MOD_EB, "Stop Create EventService");
  TcpServer tcp_server(event_service);
  tcp_server.Start();
  event_service->Run();

  return EXIT_SUCCESS;
}
