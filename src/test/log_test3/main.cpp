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
#include "log/log/log_server.h"
#include "log/log/log_client.h"
#include "app/app/app.h"
#include "app/app/appstarup.h"
#include <stdio.h>

FILE *file[10], *file_index[10];
char buffer[1110][LOG_RELEASE_MAX_LEN];

#if defined(WIN32)
const char *sformat_data[] = {
  "%s\\point%d.log", "%s\\sys%d.log", "%s\\dev%d.log", "%s\\ui%d.log"
};
const char *sformat_index[] = {
  "%s\\point%d_id.log", "%s\\sys%d_id.log", "%s\\dev%d_id.log", "%s\\ui%d_id.log"
};
#else
const char *sformat_data[] = {
  "%s/point%d.log", "%s/sys%d.log", "%s/dev%d.log", "%s/ui%d.log"
};
const char *sformat_index[] = {
  "%s/point%d_id.log", "%s/sys%d_id.log", "%s/dev%d_id.log", "%s/ui%d_id.log"
};
#endif

void openfile() {
  char buf[100];
  for (int i = 0; i < 4; i++) {
    sprintf(buf, sformat_data[i], kLogFoldPathDefault, 0);
    file[i] = fopen(buf, "wb");
    sprintf(buf, sformat_index[i], kLogFoldPathDefault, 0);
    file_index[i] = fopen(buf, "wb");
    sprintf(buf, sformat_data[i], kLogFoldPathDefault, 1);
    file[i+4] = fopen(buf, "wb");
    sprintf(buf, sformat_index[i], kLogFoldPathDefault, 1);
    file_index[i+4] = fopen(buf, "wb");
  }

  for (int i = 0; i < 8; i++) {

    fseek(file[i], 0, SEEK_SET);
    fseek(file_index[i], 0, SEEK_SET);
  }
}

void closefile() {
  for (int i = 0; i < 8; i++) {
    fclose(file[i]);
    fclose(file_index[i]);
  }
}

void gendata() {
  openfile();

  char str[100] = "testmessage";
  LOG_NODE_FILE_S tmp;
  tmp.head = 0x47;

  int increase_id = 1;

  for (int o = 0; o < 30; o++) {
    for (int i = 0; i < 4; i++) {
      sprintf(str, "testmessage,id:%d", increase_id);
      FILE *fp = file[i];
      FILE *idx = file_index[i];
      tmp.id = increase_id++;
      tmp.node.sec = tmp.id / 3;
      tmp.node.usec = tmp.id;
      tmp.node.type = (1 << i);
      tmp.node.length = sizeof(tmp.node) + strlen(str);
      if (tmp.id % 6 == 0) {
        int th = tmp.id / 6;
        if (th % 2) {
          th = -th;
        }
        tmp.node.sec += th;
      }

      int pos = ftell(fp);
      fwrite(&tmp, sizeof(tmp), 1, fp);
      fwrite(str, strlen(str), 1, fp);
      fwrite(&pos, sizeof(pos), 1, idx);
    }
  }
  for (int o = 0; o < 30; o++) {
    for (int i = 0; i < 4; i++) {
      sprintf(str, "testmessage,id:%d", increase_id);
      FILE *fp = file[i+4];
      FILE *idx = file_index[i+4];
      tmp.id = increase_id++;
      tmp.node.sec = tmp.id / 3;
      tmp.node.usec = tmp.id;
      tmp.node.type = (1 << i);
      tmp.node.length = sizeof(tmp.node) + strlen(str);
      if (tmp.id % 6 == 0) {
        int th = tmp.id / 6;
        if (th % 2) {
          th = -th;
        }
        tmp.node.sec += th;
      }

      int pos = ftell(fp);
      fwrite(&tmp, sizeof(tmp), 1, fp);
      fwrite(str, strlen(str), 1, fp);
      fwrite(&pos, sizeof(pos), 1, idx);
    }
  }

  closefile();
}

void print_data(char buffer[][LOG_RELEASE_MAX_LEN], int len) {

  for (int i = 0; i < len; i++) {
    LOG_NODE_S *node = (LOG_NODE_S *)buffer[i];
    printf("[%d][len=%d][type=%d][%d:%d] - \"%s\"\n"
           , i + 1, node->length, node->type
           , node->sec, node->usec, (char *)(&buffer[i][sizeof(LOG_NODE_S)]));
  }
}

void query_test(LOG_TIME_S st, LOG_TIME_S ed,
                uint8 log_mask, LOG_QUERY_TYPE type,
                const char *name) {
  printf("query_test \"%s\" start\n", name);
  uint32 start_id, last_id;
  LOG_QUERY_NODE qnode;
  qnode.last_id = 0;
  qnode.qtype = type;
  int tot = 0;
  for (int i = 0;; i++) {
    memset(buffer, 0, sizeof(buffer));
    if (i == 0) {
      qnode.is_first = true;
    } else {
      qnode.is_first = false;
      qnode.start_id = qnode.last_id;
    }
    int res = Log_Search(log_mask, st, ed, 10, buffer, qnode);
    if (res == 0) {
      break;
    }
    printf("query round %d start\n", i);
    print_data(buffer, res);
    printf("query round %d end\n", i);
    tot += res;
  }
  printf("query_test \"%s\" end\n", name);
  printf("-------------find %d res------------\n\n", tot);
}


class LogClientApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<LogClientApp>,
  public sigslot::has_slots<> {
 public:
  LogClientApp() : AppInterface("LogClientApp") {
  }
  virtual ~LogClientApp() {
  }
  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    //event_service_ = event_service;
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    char buf[1024];
    int cnt = 0;
    int mid;
    LOG_SRV_CONFIG_S config;
    clock_t start_clock;
    clock_t end_clock;
#define TIME_USDE_TEST_CNT 3
    double time_used[TIME_USDE_TEST_CNT];
    char tmpstr[100];
    char c;

    printf("LogClientApp is running...\n");

    mid = Log_DbgModRegist("debug_test", LL_DEBUG);

    Log_SrvConfigGet(&config);
    for (int i = 0; i < LOG_SRV_FILE_TYPE_MAX; i++) {
      config.file_maxlen[i] = 1 * 1024;
      config.cache_maxsize[i] = 256;
      config.write_overtime_cnt[i] = 1;
    }
    Log_SrvConfigSet(&config);
    Log_SetRemoteServer(0xc0a80806, 5000); // 192.168.8.6
    //Log_SetPrintEnd(LT_DEBUG, LE_ALL);
    Log_SetPrintEnd(LT_SYS, LE_REMOTE);
    Log_DbgSetLevel(MOD_EB, LL_NONE);

    uint32 counts = 0;

    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  //vzes::EventService::Ptr event_service_;
  int cnt_;
};


int main(int argc, char *argv[]) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr logclientapp(new LogClientApp());
  app->RegisterApp(logclientapp);
  gendata();

  LOG_TIME_S st, ed;
  uint8 mask;
  if(argc < 5) {
    puts("use default para");
    st.sec = 0;
    st.usec = 0;
    ed.sec = ~0;
    ed.usec = ~0;
  } else {
    st.sec = atoi(argv[1]);
    st.usec = atoi(argv[2]);
    ed.sec = atoi(argv[3]);
    ed.usec = atoi(argv[4]);
  }
  if(argc != 6) {
    puts("use default mask");
    mask = 0xff;
  } else {
    mask = atoi(argv[5]);
  }
  Log_DbgSetLevel(MOD_EB, LL_NONE);
  printf("st: %u : %u ed: %u : %u mask:%x\n", st.sec, st.usec, ed.sec, ed.usec, mask);

  query_test(st, ed, mask, LOG_QUERY_ASC_TYPE, "sec");
  query_test(st, ed, mask, LOG_QUERY_DESC_TYPE, "desc");

  getchar();
  //query_test(st, ed, 0xff, LOG_QUERY_ASC_TYPE, "asc all");
  //query_test(st, ed, 0xf3, LOG_QUERY_DESC_TYPE, "desc all");
  app->ExitApp();
  return EXIT_SUCCESS;
}