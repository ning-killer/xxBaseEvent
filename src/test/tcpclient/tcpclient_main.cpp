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

#include "base64.hpp"
#include "json/json.h"
#include <iostream>
#include <stdio.h>
#include "eventservice/net/eventservice.h"
#include "eventservice/net/eventservice.h"
#include "log/log/log_client.h"
#include "filecache/cache/cacheclient.h"


#ifdef WIN32
#define FC_FILE_PATH   "/filecache_test_1.txt"
#else
#define FC_FILE_PATH   "/filecache_test_1.txt"
#endif

const char NOT_FOUND[] =
  "HTTP/1.0 200 OK\r\n"
  "Server:Apache Tomcat/5.0.12\r\n"
  "Content-Type:text/html\r\n\r\n"
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "404 Not Found</h1></body>"
  "</html>";

const char test_buf[] = { 0x56, 0x5A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x31, 0x32, 0x33 };

uint32_t ntol(uint32_t val) {
  char r[4];
  char *v = (char *)&val;
  r[0] = v[3];
  r[1] = v[2];
  r[2] = v[1];
  r[3] = v[0];
  return *(uint32_t *)r;
}

class TcpClient : public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<TcpClient>,
  public sigslot::has_slots<> {
 public:
  TcpClient(vzes::EventService::Ptr event_service)
    : event_service_(event_service),
      pack_cnt_(0) {
  }
  bool Start() {
    ASSERT_RETURN_FAILURE(async_client_, false);

    cache_client_ = cache::CacheClient::CreateCacheClient();
    async_client_ = event_service_->CreateAsyncConnect();

    vzes::SocketAddress address("192.168.18.137", 8099);
    async_client_->SignalServerConnected.connect(
      this, &TcpClient::OnConnectEvent);

    return async_client_->Connect(address, 10000);
  }

 private:
  void OnConnectEvent(vzes::AsyncConnecter::Ptr client,
                      vzes::Socket::Ptr s,
                      int err) {

    if (err != 0 || !s) {
      DLOG_ERROR(MOD_EB, "socket connect remote failed");
      return ;
    }

    DLOG_INFO(MOD_EB, "socket connect romote successed:%d", err);
    int s_ = ::socket(AF_INET, TCP_SOCKET, 0);
    vzes::SocketAddress address("192.168.18.137", 8099);
    sockaddr_storage addr_storage;
    size_t len = address.ToSockAddrStorage(&addr_storage);
    sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
    int error = ::connect(s_, addr, static_cast<int>(len));
    vzes::AsyncSocket::Ptr socket = event_service_->CreateAsyncSocket(s_);
    socket->SignalSocketWriteEvent.connect(
      this, &TcpClient::OnSocketWriteComplete);
    socket->SignalSocketReadEvent.connect(
      this, &TcpClient::OnSocketReadComplete);
    socket->SignalSocketErrorEvent.connect(
      this, &TcpClient::OnSocketErrorEvent);

    async_sockets_.push_back(socket);

    char full_path[128] = {0};
    socket->SetEncodeType(vzes::PKT_ENCODE_BASE64);
    cache_client_->Write(FC_FILE_PATH, NOT_FOUND, strlen(NOT_FOUND), full_path);

    // write is Async operations, so here sleep 200ms, to wait write done
    vzsleep(200);

    // 读取文件
    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(full_path);
    vzes::BlocksPtr &blocks = read_buffer->blocks();
    read_buffer->EnableEncode(true);

    //char m_data_head_[] = {};
    //socket->AsyncWrite(m_data_head_, sizeof(m_data_head_));

    Json::Value v_value;
    //v_value["cmd"] = "set_tcpconn";
    //v_value["s_ip"] = "192.168.1.141";
    //v_value["s_port"] = 5544; //server
    //v_value["s_port"] = 8097; //工具

    //v_value["cmd"] = "query_face";
    //v_value["id"] = 1;

    //v_value["cmd"] = "delete_face";
    //v_value["type"] = 0;
    //v_value["per_id"] = "0005";

    //v_value["cmd"] = "evs_get_rs485";
    //v_value["body"]["source"] = 0;

    //v_value["cmd"] = "send_rs485";
    //v_value["body"]["source"] = 0;
    //v_value["body"]["data"] = "123456";

    //v_value["cmd"] = "evs_open_rs485";
    //v_value["body"]["source"] = 0;
    //v_value["body"]["baud_rate"] = 9600;
    //v_value["body"]["parity_id"] = 0;
    //v_value["body"]["data_bits"] = 8;
    //v_value["body"]["stop_bits"] = 1;

    //v_value["cmd"] = "set_led_cfg";
    //v_value["body"]["mode"] = 1;
    //v_value["body"]["level"] = 100;

    //v_value["cmd"] = "set_cluster_time";
    //v_value["body"]["time"] = 5;

    //v_value["cmd"] = "set_match_score";
    //v_value["body"]["score"] = 50;

    //std::string v_string(R"({ "cmd":"create_face", "img_data" : "dGVzdCBpbWc=", "per_id" : "0002", "per_name" : "pony", "version" : "0.2" })");
    char *buf = (char*)malloc(1024 * 1024);
    char *buf_base64 = (char*)malloc(1024 * 1024);
    const char path[] = "F://RX/pic/flash_path/0002_6vyhGteSOyU3.jpg";
    FILE *fp = fopen(path, "rb");
    if (NULL == fp) {
      return ;
    }
    fseek(fp, 0L, SEEK_END);
    int buf_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    int read_size = fread((void*)buf, 1, buf_size, fp);
    if (read_size != buf_size) {
      fclose(fp);
      return ;
    }
    fclose(fp);

    //Json::Value v_value;
    //v_value["cmd"] = "create_face";
    //v_value["per_id"] = "0006";
    //v_value["per_name"] = "dcy";
    //v_value["version"] = "0.2";
    ////0002_6vyhGteSOyU3.jpg
    //tbase64_encode((const unsigned char*)buf, buf_base64, buf_size);
    //v_value["img_data"] = buf_base64;

    char data_head[12] = { 0 };
    data_head[0] = 'V';
    data_head[1] = 'Z';
    data_head[2] = 0x00;
    data_head[3] = 0x00;

    vzstd::string v_string = v_value.toStyledString();

    *(uint32_t *)(data_head + 8) = ntol(v_string.size());
    bool res;
    //for (int i = 0; i < 5; i++) {
    socket->AsyncWrite(data_head, sizeof(data_head));
    res = socket->AsyncWrite(v_string.c_str(), v_string.size());
    //}
    if (!res) {
      DLOG_ERROR(MOD_EB, "send data failed");
    }

    socket->AsyncRead();
    //event_service_->PostDelayed(5*1000, this, 0);
  }

  void OnSocketWriteComplete(vzes::AsyncSocket::Ptr async_socket) {
    //DLOG_INFO(MOD_EB, "send data done");
  }

  void OnSocketReadComplete(vzes::AsyncSocket::Ptr async_socket,
                            vzes::MemBuffer::Ptr data) {
    char *recv_buf = (char *)malloc(1024 * 1024);
    data->CopyBytes(recv_buf, 0, data->size());
    char *recv_buf_data = recv_buf + 8;
    //DLOG_INFO(MOD_EB, "received data, size:%d", data->size());
    //pack_cnt_ ++;
    //pack_size_ += data->size();
    //async_socket->AsyncWrite(NOT_FOUND, strlen(NOT_FOUND));

    //Json::Value v_value;
    //char data_head[12] = { 0 };
    //data_head[0] = 'V';
    //data_head[1] = 'Z';
    //data_head[2] = 0x00;
    //data_head[3] = 0x00;

    //vzstd::string v_string = v_value.toStyledString();

    //*(uint32_t *)(data_head + 8) = ntol(v_string.size());
    //bool res;
    ////for (int i = 0; i < 5; i++) {
    //async_socket->AsyncWrite(data_head, sizeof(data_head));
    //res = async_socket->AsyncWrite(v_string.c_str(), v_string.size());
    async_socket->AsyncRead();

  }

  void OnSocketErrorEvent(vzes::AsyncSocket::Ptr async_socket,
                          int err) {
    async_socket->Close();
    async_sockets_.remove(async_socket);
  }

  virtual void OnMessage(vzes::Message *msg) {

    Json::Value v_value;
    char data_head[12] = { 0 };
    data_head[0] = 'V';
    data_head[1] = 'Z';
    data_head[2] = 0x00;
    data_head[3] = 0x00;

    vzstd::string v_string = v_value.toStyledString();

    *(uint32_t *)(data_head + 8) = ntol(v_string.size());
    bool res;
    std::list<vzes::AsyncSocket::Ptr>::iterator iter;
    for (iter = async_sockets_.begin(); iter != async_sockets_.end(); iter++) {
      (*iter)->AsyncWrite(data_head, sizeof(data_head));
      (*iter)->AsyncWrite(v_string.c_str(), v_string.size());
    }
    event_service_->PostDelayed(5*1000, this, 0);
  }

 private:
  uint32                                     pack_cnt_;
  uint32                                     pack_size_;
  vzes::EventService::Ptr                    event_service_;
  vzes::AsyncConnecter::Ptr                  async_client_;
  cache::CacheClient::Ptr                    cache_client_;
  typedef std::list<vzes::AsyncSocket::Ptr>  AsyncSockets;
  AsyncSockets                               async_sockets_;
};

int main(void) {
  // Initialize the logging system
  (void)Log_Init(false);
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("TcpClient");
  TcpClient tcp_client(event_service);
  tcp_client.Start();
  event_service->Run();

  return EXIT_SUCCESS;
}
