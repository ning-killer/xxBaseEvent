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
#include <stdlib.h>
#include <time.h>
#include "log/log/log_client.h"
#include "log/log/log_server.h"
#include "app/app/app.h"
#include "app/app/appstarup.h"

#include "eventservice/base/timeutils.h"

#define LOG_WRITE_TEST 0

class LogClientApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<LogClientApp>,
  public sigslot::has_slots<> {
 public:
  LogClientApp() : AppInterface("LogClientApp") {
  }
  virtual ~LogClientApp() {
  }


  void Log_write(LOG_TYPE_E type, char *msg) {
#if LOG_WRITE_TEST
    do {
      if (LOG_TRACE(type, msg))
        break;
      vzsleep(1);
    } while (1);
#else
    LOG_TRACE(type, msg);
#endif
  }

  void Log_read() {
    LOG_TIME_S starttime;
    LOG_TIME_S endtime;
    uint32 last_seek_id = 0;
    uint32 max_id = 0;
#define ONCE_READ_MAXCNT 15
    char buf[ONCE_READ_MAXCNT][256];
    int rtn_cnt;
    int tot_cnt = 0;

    starttime.sec = 0;
    starttime.usec = 0;
    endtime.sec = 0xffffffff;
    endtime.usec = 0xffffffff;
    LOG_QUERY_NODE qnode;
    qnode.last_id = 0;

    do {
      memset(buf, 0, sizeof(buf));
      qnode.start_id = qnode.last_id;
      rtn_cnt = Log_Search(0xff, starttime, endtime,
                           ONCE_READ_MAXCNT, buf,
                           qnode);
      printf("\n--rtn_cnt[%d]---\n", rtn_cnt);
      for (int i = 0; i < rtn_cnt; i++) {
        LOG_NODE_S *node = (LOG_NODE_S *)buf[i];
        printf("[%d][len=%d][type=%d][%d:%d] - \"%s\"\n"
               , i + 1, node->length, node->type
               , node->sec, node->usec, (char *)(&buf[i][sizeof(LOG_NODE_S)]));
      }
      tot_cnt += rtn_cnt;
    } while (rtn_cnt >= ONCE_READ_MAXCNT);
    printf("max_id = %u\n", qnode.max_id);
    printf("\t---- tot_cnt = %d\n", tot_cnt);
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
    Log_SetPrintEnd(LT_DEBUG, LE_LOCAL | LE_FILE);
    Log_SetPrintEnd(LT_SYS, LE_REMOTE);

    uint32 counts = 0;
    while (1) {
#if 0
      scanf("%s", tmpstr);
      c = tmpstr[0];
      //c = 'd';
#else
      c = (counts++ % 5) + '1';
      sprintf(buf, "%03d", cnt++);
#endif

      if (c >= '1' && c <= '5') {
        LOG_TYPE_E type = (LOG_TYPE_E)(1 << (c - '1'));
        if (type == LT_DEBUG) {
          sprintf(buf, "Debug test=%05d", cnt);
          Log_write(type, buf);
        } else {
          Log_write(type, buf);
        }
#if LOG_WRITE_TEST // �������
        vzsleep(3000);
        for (int itest = 0; itest < 2; itest++) {
          if (itest == 0) {
            type = LT_DEBUG;
          } else {
            type = LT_POINT;
          }
          for (int i = 0; i < TIME_USDE_TEST_CNT; i++) {
            cnt = 1;
            start_clock = clock();
            for (int j = 0; j < 10000; j++) {
              sprintf(buf, "05%d", cnt++);
              Log_write(type, buf);
            }
            end_clock = clock();
            time_used[i] = (double)(end_clock - start_clock) / CLOCKS_PER_SEC;
          }
          vzsleep(100);
          printf("\n-------- Test_%d -----------\n", itest + 1);
          for (int i = 0; i < TIME_USDE_TEST_CNT; i++) {
            printf("--%d. Use time is: %.8f\n", i, time_used[i]);
          }
        }
        break;
#endif
        vzsleep(200);
        printf("Log_write Down.\n");
      } else if (c == 'r') {
        Log_read();
      } else if (c == 'd') {
        DLOG_DEBUG(mid, "Debug_Message1");
        DLOG_DEBUG(mid, "Debug_Message2");
        DLOG_DEBUG(mid, "Debug_Message3");
        DLOG_INFO(mid, "Info_Message");
        DLOG_WARNING(mid, "Waring_Message");
        DLOG_ERROR(mid, "Error_Message");
        DLOG_KEY(mid, "Key_Message");
        Log_DbgTrace(mid, LL_NONE, __FILE__, __LINE__, "None_Message");

        vzsleep(1000);
        Log_SetPrintEnd(LT_DEBUG, LE_OFF);
        DLOG_DEBUG(mid, __FILE__, __LINE__, "!Error, I need hide!");
        vzsleep(1000);
        Log_SetPrintEnd(LT_DEBUG, LE_LOCAL | LE_FILE);
        Log_DbgTrace(mid, LL_KEY, __FILE__, __LINE__, "Good, I can see.");
      } else if (c == 'p') {
        memset(buf, 'A', 1024);
        buf[128] = '\0';
        printf("Batch[1000 time - 10s] write file...\n");
        for (int i = 0; i < 100; i++) {
          Log_write(LT_POINT, buf);
          Log_write(LT_SYS, buf);
          vzsleep(100);
          if (i % 10 == 0) {
            printf("%d\n", i);
          }
        }
        printf("-- finish --\n");
      } else if (c == 's') {
        Log_SetPrintEnd(LT_UI, LE_LOCAL | LE_FILE);
        Log_write(LT_UI, "-----------Good, I can see.-----------");
        vzsleep(1000);
        Log_write(LT_UI, "***********Error, I need hide.***********");
        Log_SetPrintEnd(LT_UI, LE_OFF);
      } else if (c == 'q') {
        exit(0);
      }
    }
    //vzsleep(1000);

    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  //vzes::EventService::Ptr event_service_;
  int cnt_;
};

class LogSearchApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<LogSearchApp>,
  public sigslot::has_slots<> {
 public:
  LogSearchApp() : AppInterface("LogSearchApp") {
  }
  virtual ~LogSearchApp() {
  }

  void Log_write(LOG_TYPE_E type, char *msg) {
    LOG_TRACE(type, msg);
  }

  void Log_read() {
    LOG_TIME_S starttime;
    LOG_TIME_S endtime;
    uint32 last_seek_id = 0;
    uint32 max_id;
    char buf[15][256];
    int rtn_cnt;

    starttime.sec = 0;
    starttime.usec = 0;
    endtime.sec = 0xffffffff;
    endtime.usec = 0xffffffff;
    LOG_QUERY_NODE qnode;
    qnode.last_id = 0;

    int tot_cnt = 0;
    qnode.is_first = true;
    do {
      memset(buf, 0, sizeof(buf));
      qnode.start_id = qnode.last_id;
      rtn_cnt = Log_Search(0xff, starttime, endtime,
                           15, buf,
                           qnode);
      qnode.is_first = false;
#if 0
      printf("\n--rtn_cnt[%d]---\n", rtn_cnt);
      for (int i = 0;
           i < rtn_cnt;
           i++) {
        LOG_NODE_S *node = (LOG_NODE_S *)buf[i];
        printf("[%d][len=%d][type=%d][%d:%d] - \"%s\"\n"
               , i + 1, node->length, node->type
               , node->sec, node->usec, (char *)(&buf[i][sizeof(LOG_NODE_S)]));
      }
#endif
      tot_cnt += rtn_cnt;
    } while (rtn_cnt >= 15);
    printf("--------max_id = %u\n", qnode.max_id);
    printf("\t---- tot_cnt = %d\n", tot_cnt);
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
    printf("LogSearchApp is running...\n");
    vzsleep(5000);
    while (1) {
      Log_read();
      vzsleep(5000);
    }
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  //vzes::EventService::Ptr event_service_;
  int cnt_;
};

void PrintNode(LOG_NODE_S *node) {
  vzes::TimeLocal tm;
  vzes::TimeMkLocal(&tm, node->sec);

  uint8 *p = ((uint8 *)node) + sizeof(LOG_NODE_S);
  const char *kLogLevelString[] = { "DEBUG", "INFO", "WARNING"
                                    , "ERROR", "KEY", "-"
                                  };
  const char *kLogSrcString[] = {
    "POINT", "SYS", "DEV", "UI"
  };

#define NODE_PRINT() printf("%04d-%02d-%02d %02d:%02d:%02d,%-6d,[%s]: %s\n" \
  ,tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec \
  , kLogLevelString[node->level], p)
#define NODE_PRINT_C(COLOR) printf("\033[1;%d;40m%04d-%02d-%02d %02d:%02d:%02d,%-6d,[%s]: %s\033[0m\n" \
  , COLOR,tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec \
  , kLogLevelString[node->level], p)
#define NODE_PRINT_N() printf("%04d-%02d-%02d %02d:%02d:%02d,%-6d: %s\n" \
  ,tm.year, tm.month, tm.day, tm.hour, tm.min, tm.sec, node->usec \
  , p)

  NODE_PRINT_N();
}

#define err_println(fmt, ...) printf("\t\033[1;31;40m[Error:] " fmt "\033[0m\n", ##__VA_ARGS__)

void PrintLogOnlyIndex(const char *index_filename) {
  FILE *f_index;
  uint8 buf[2048];
  uint32 offset;
  size_t rdlen;
  int index_len;
  int cnt = 0;
  uint32 last_offset = 0;

  // index
  f_index = fopen(index_filename, "rb");
  if (f_index == NULL) {
    err_println("Invalid index file = \"%s\"", index_filename);
    return;
  }
  fseek(f_index, 0, SEEK_END);
  index_len = ftell(f_index);
  fseek(f_index, 0, SEEK_SET);
  printf("FileLength: index=%d.\n", index_len);

  while (1) {
    rdlen = fread(buf, 1, 4, f_index);
    if (rdlen != 4) {
      printf("\n--- Index end, read cnt = %d.---\n", cnt);
      if (rdlen > 0) {
        err_println("(%d) - Index more bytes = %d.", (int)ftell(f_index), rdlen);
      }
      fclose(f_index);
      return;
    } else {
      offset = *((uint32 *)buf);
    }
    printf("%05d:%07d  ", cnt + 1, offset);
    if (offset < last_offset) {
      printf("\n");
      err_println("(%d) - Offset!", (int)offset);
      fclose(f_index);
      return;
    }
    if (cnt % 6 == 5) {
      printf("\n");
    }
    last_offset = offset;
    cnt++;
  }
  fclose(f_index);
}

void PrintLogOnlyData(const char *data_filename) {
  LOG_NODE_FILE_S *item;
  char filename[2][128];
  FILE *f_data;
  FILE *f_index;
  uint8 buf[2048];
  uint32 offset;
  size_t rdlen;
  int data_len;
  int index_len;
  int cnt = 0;
  int content_len;
  int start_offset;
  int cur_offset;
  int last_s = 0;
  int last_us = 0;

  // data
  f_data = fopen(data_filename, "rb");
  if (f_data == NULL) {
    err_println("Invalid data file = \"%s\"\n", data_filename);
    return;
  }
  fseek(f_data, 0, SEEK_END);
  data_len = ftell(f_data);
  fseek(f_data, 0, SEEK_SET);
  printf("FileLength: data=%d.\n", data_len);

  start_offset = 0;
  cur_offset = 0;
  while (1) {
    rdlen = fread(buf, 1, 1, f_data);
    if (rdlen != 1) {
      printf("--- Data end, read cnt = %d.---\n", cnt);
      cur_offset -= start_offset;
      if (cur_offset > 0) {
        err_println("(%d) - Data more bytes = %d.", start_offset, cur_offset);
      }
      return;
    }
    if (buf[0] != LOG_FILE_NODE_HEAD) {
      cur_offset++;
      continue;
    }
    if (start_offset != cur_offset) {
      err_println("(%d) - Data bypass bytes = %d.", start_offset, cur_offset - start_offset);
    }
    rdlen = fread(&buf[1], sizeof(LOG_NODE_FILE_S) - 1, 1, f_data);
    if (rdlen != 1) {
      err_println("(%d) - Data head read.", (int)offset);
      fclose(f_data);
      return;
    }
    item = (LOG_NODE_FILE_S *)buf;
    if (item->head != LOG_FILE_NODE_HEAD) {
      err_println("(%d) - Head flag = 0x%02X.", (int)offset, item->head);
      fclose(f_data);
      return;
    }
    if (item->node.length > LOG_RELEASE_MAX_LEN) {
      err_println("(%d) - node_length too long = %d.", (int)offset, data_len - offset, item->node.length);
      fclose(f_data);
      return;
    }
    if (offset + 1 + item->node.length + 2 > data_len) {
      err_println("(%d) - Less content data, only length = %d, node_length = %d.", (int)offset, data_len - offset, item->node.length);
      fclose(f_data);
      return;
    }
    content_len = item->node.length - sizeof(LOG_NODE_FILE_S) + 1 + 2;
    rdlen = fread(buf + sizeof(LOG_NODE_FILE_S), 1, content_len, f_data);
    if (rdlen != content_len) {
      err_println("(%d) - Data content read, need=%d, read=%d.", (int)offset, content_len, rdlen);
      fclose(f_data);
      return;
    }
//  for (int i = 0; i < 16; i++) {
//    printf("%02X ", buf[i]);
//    if (I % 4 == 3) {
//      printf("  ");
//    }
//  }
//  printf("\n");
    PrintNode(&item->node);
    if ((last_s > item->node.sec) || (last_s == item->node.sec && last_us >= item->node.usec)) {
      err_println("(%d) - Time!", (int)offset);
      start_offset = 0;
//    fclose(f_data);
//    return;
    }
    last_s = item->node.sec;
    last_us = item->node.usec;
    cnt++;
    cur_offset += 1 + item->node.length + 2;
    start_offset = cur_offset;
  }
  fclose(f_data);
}

void PrintLog(const char *data_filename, const char *index_filename) {
  LOG_NODE_FILE_S *item;
  char filename[2][128];
  FILE *f_data;
  FILE *f_index;
  uint8 buf[2048];
  uint32 offset;
  size_t rdlen;
  int data_len;
  int index_len;
  int cnt = 0;
  int content_len;
  int last_s = 0;
  int last_us = 0;

  // data
  f_data = fopen(data_filename, "rb");
  if (f_data == NULL) {
    err_println("Invalid data file = \"%s\"", data_filename);
    return;
  }
  fseek(f_data, 0, SEEK_END);
  data_len = ftell(f_data);
  fseek(f_data, 0, SEEK_SET);
  // index
  f_index = fopen(index_filename, "rb");
  if (f_index == NULL) {
    fclose(f_data);
    err_println("(%d) -Invalid index file = \"%s\"", index_filename);
    return;
  }
  fseek(f_index, 0, SEEK_END);
  index_len = ftell(f_index);
  fseek(f_index, 0, SEEK_SET);
  printf("FileLength: data=%d, index=%d.\n", data_len, index_len);

  while (1) {
    rdlen = fread(buf, 1, 4, f_index);
    if (rdlen != 4) {
      printf("--- Index end, read cnt = %d.---\n", cnt);
      if (rdlen > 0) {
        err_println("(%d) - Index more bytes = %d.", (int)ftell(f_index), rdlen);
      }
      offset = ftell(f_data);
      if (offset < data_len) {
        err_println("(%d) - Data more bytes = %d.", (int)ftell(f_index), data_len - offset);
      }
      return;
    } else {
      offset = *((uint32 *)buf);
    }
    if (offset + sizeof(LOG_NODE_FILE_S) > data_len) {
      err_println("(%d) - Less node data, only length = %d.", (int)ftell(f_index), data_len - offset);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    fseek(f_data, offset, SEEK_SET);
    rdlen = fread(buf, sizeof(LOG_NODE_FILE_S), 1, f_data);
    if (rdlen != 1) {
      err_println("(%d) - Data head read.", (int)offset);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    item = (LOG_NODE_FILE_S *)buf;
    if (item->head != LOG_FILE_NODE_HEAD) {
      err_println("(%d) - Head flag = 0x%02X.", (int)offset, item->head);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    if (item->node.length > LOG_RELEASE_MAX_LEN) {
      err_println("(%d) - node_length too long = %d.", (int)offset, data_len - offset, item->node.length);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    if (offset + 1 + item->node.length + 2 > data_len) {
      err_println("(%d) - Less content data, only length = %d, node_length = %d.", (int)offset, data_len - offset, item->node.length);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    content_len = item->node.length - sizeof(LOG_NODE_FILE_S) + 1 + 2;
    rdlen = fread(buf + sizeof(LOG_NODE_FILE_S), 1, content_len, f_data);
    if (rdlen != content_len) {
      err_println("(%d) - Data content read, need=%d, read=%d.", (int)offset, content_len, rdlen);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    rdlen = *((uint16 *)(buf + 1 + item->node.length));
    if (rdlen != item->node.length) {
      err_println("(%d) - Data length, head=%d, tail=%d.", item->node.length, rdlen);
      fclose(f_data);
      fclose(f_index);
      return;
    }
    PrintNode(&item->node);
    if ((last_s > item->node.sec) || (last_s > item->node.sec && last_us >= item->node.usec)) {
      err_println("(%d) - cnt=%d Time!", (int)offset, cnt);
//    fclose(f_data);
//    fclose(f_index);
//    return;
    }
    last_s = item->node.sec;
    last_us = item->node.usec;
    cnt++;
  }
  fclose(f_data);
  fclose(f_index);
}

//  ./logtest /tmp/vzlog/point0.log /tmp/vzlog/point0_id.log
//int vzes_app_main(int argc, char *argv[]) {
int main(int argc, char *argv[]) {
  char *default_data_path[] = {
    "/tmp/vzlog/point0.log", "/tmp/vzlog/dev0.log", "/tmp/vzlog/sys0.log", "/tmp/vzlog/ui0.log"
  };
  char *default_index_path[] = {
    "/tmp/vzlog/point0_id.log", "/tmp/vzlog/dev0_id.log", "/tmp/vzlog/sys0_id.log", "/tmp/vzlog/ui0_id.log"
  };
  bool is_write = true;
  bool is_read = true;

  printf("Example run: ./logtest data_file_path {index_file_path}\n\n");
  do {
    if (argc > 1) {
      if (argc == 2) {
        if (strcmp(argv[1], "data") == 0) {
          for (int i = 0; i < 4; i++) {
            printf("---------------- %s -----------------\n", default_data_path[i]);
            PrintLogOnlyData(default_data_path[i]);
          }
        } else if (strcmp(argv[1], "all") == 0) {
          for (int i = 0; i < 4; i++) {
            printf("---------------- %s -----------------\n", default_index_path[i]);
            PrintLog(default_data_path[i], default_index_path[i]);
          }
        } else if (strcmp(argv[1], "index") == 0) {
          for (int i = 0; i < 4; i++) {
            printf("---------------- %s -----------------\n", default_index_path[i]);
            PrintLogOnlyIndex(default_index_path[i]);
          }
        } else if (strcmp(argv[1], "check") == 0) {
          app::App::Ptr app = app::App::CreateApp();
          app->AppRun();
          app->ExitApp();
          return EXIT_SUCCESS;
        } else if (strcmp(argv[1], "r") == 0) {
          is_write = false;
          break;
        } else if (strcmp(argv[1], "w") == 0) {
          is_read = false;
          break;
        } else
          PrintLogOnlyData(argv[1]);
      } else if (argc == 3) {
        PrintLog(argv[1], argv[2]);
      }
      return 0;
    }
  } while (0);
  printf("log_test_main is running...\n");
  app::App::Ptr app = app::App::CreateApp();
  if (is_write) {
    app::AppInterface::Ptr logclientapp(new LogClientApp());
    app->RegisterApp(logclientapp);
  }
  if (is_read) {
    app::AppInterface::Ptr logSearchapp(new LogSearchApp());
    app->RegisterApp(logSearchapp);
  }
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}