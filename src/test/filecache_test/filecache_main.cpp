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
#include <stdio.h>
#include <iostream>
#include "app/app/app.h"
#include "app/app/appstarup.h"
#include "filecache/kvdb/kvdbclient.h"
#include "filecache/cache/cacheclient.h"
#include "filecache/kvdb/kvdbclient_c.h"
#include "eventservice/base/common.h"
#include "eventservice/base/timeutils.h"
#include "eventservice/base/basicincludes.h"
#include "eventservice/base/base64.h"


#define KVDB_KEY_HELLO     "Hello"
#define KVDB_VALUE_HELLO   "Hello, this is kvdb test case!"
#define KVDB_KEY_WORLD     "World"
#define KVDB_VALUE_WORLD   "World, this is kvdb test case!"

#ifdef WIN32
#define FC_FILE_PATH_1     "filecache_test_1.txt"
#define FC_FILE_PATH_2     "filecache_test_2.txt"
#define FC_FILE_PATH_3     "filecache_test_3.txt"
#define FC_FILE_PATH_4     "filecache_test_4.txt"
#define FC_FILE_PATH_5     "filecache_test_5.txt"
#define FC_FILE_PATH_6     "filecache_test_6.txt"
#else
#define FC_FILE_PATH_1     "filecache_test_1.txt"
#define FC_FILE_PATH_2     "filecache_test_2.txt"
#define FC_FILE_PATH_3     "filecache_test_3.txt"
#define FC_FILE_PATH_4     "filecache_test_4.txt"
#define FC_FILE_PATH_5     "filecache_test_5.txt"
#define FC_FILE_PATH_6     "filecache_test_6.txt"
#endif

char fc_content[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

class KvdbApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<KvdbApp>,
  public sigslot::has_slots<> {
 public:
  KvdbApp() :AppInterface("KvdbApp") {
  }
  virtual ~KvdbApp() {
  }

  void kvdb_test_case_1() {
    DLOG_INFO(MOD_EB, ">>> kvdb test case 1 start");
    vzstd::string value;
    kvdb_client_->GetKey(KVDB_KEY_HELLO, &value);
    if (!strcmp((const char*)KVDB_VALUE_HELLO, (const char*)value.c_str())) {
      DLOG_INFO(MOD_EB, "kvdb test case 1 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 1 failed -_-, "
                "set value: %s, get value: %s",
                (const char*)KVDB_VALUE_HELLO, value.c_str());
    }
    kvdb_client_->GetKey(KVDB_KEY_HELLO, &value);
    if (!strcmp((const char*)KVDB_VALUE_HELLO, (const char*)value.c_str())) {
      DLOG_INFO(MOD_EB, "kvdb test case 1 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 1 failed -_-, "
                "set value: %s, get value: %s",
                (const char*)KVDB_VALUE_HELLO, value.c_str());
    }
    kvdb_client_->SetKey(KVDB_KEY_HELLO,
                         KVDB_VALUE_HELLO,
                         strlen(KVDB_VALUE_HELLO));
    kvdb_client_->GetKey(KVDB_KEY_HELLO, &value);
    if (!strcmp((const char*)KVDB_VALUE_HELLO, (const char*)value.c_str())) {
      DLOG_INFO(MOD_EB, "kvdb test case 1 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 1 failed -_-, "
                "set value: %s, get value: %s",
                (const char*)KVDB_VALUE_HELLO, value.c_str());
    }

    DLOG_INFO(MOD_EB, ">>> kvdb test case 1 end");
  }

  void kvdb_test_case_2() {
    // C接口测试
    DLOG_INFO(MOD_EB, ">>> kvdb test case 2 start");
    bool successd = false;

    do {
      KVDB_HANDLE kvdb_c = Kvdb_Create("ckvdb");
      if (NULL == kvdb_c) {
        DLOG_INFO(MOD_EB, "create kvdb instance failed!!!");
        break;
      }

      bool result = Kvdb_SetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                                KVDB_VALUE_HELLO, strlen(KVDB_VALUE_HELLO));
      if (!result) {
        DLOG_INFO(MOD_EB, "set key failed!!!");
        break;
      }

      char buffer[1024] = {0};
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (!result) {
        DLOG_INFO(MOD_EB, "get key failed!!!");
        break;
      }

      if (strcmp((const char*)KVDB_VALUE_HELLO, buffer)) {
        DLOG_INFO(MOD_EB, "key value incorrect, "
                  "set value: %s, get value: %s",
                  (const char*)KVDB_VALUE_HELLO, buffer);
        break;
      }

      successd = true;
    } while (0);

    if (successd) {
      DLOG_INFO(MOD_EB, "kvdb test case 2 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 2 failed -_-");
    }

    DLOG_INFO(MOD_EB, ">>> kvdb test case 2 end");
  }

  void kvdb_test_case_3() {
    // C接口测试
    DLOG_INFO(MOD_EB, ">>> kvdb test case 3 start");
    bool successd = false;

    do {
      KVDB_HANDLE kvdb_c = Kvdb_Create("ckvdb");
      if (NULL == kvdb_c) {
        DLOG_INFO(MOD_EB, "create kvdb instance failed!!!");
        break;
      }

      bool result = Kvdb_SetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                                KVDB_VALUE_HELLO, strlen(KVDB_VALUE_HELLO));
      if (!result) {
        DLOG_INFO(MOD_EB, "set key failed!!!");
        break;
      }

      result = Kvdb_DeleteKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO));
      if (!result) {
        DLOG_INFO(MOD_EB, "delete key failed!!!");
        break;
      }

      char buffer[1024] = {0};
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (result) {
        DLOG_INFO(MOD_EB, "get key failed, this key is deleted!!!");
        break;
      }

      successd = true;
    } while (0);

    if (successd) {
      DLOG_INFO(MOD_EB, "kvdb test case 3 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 3 failed -_-");
    }

    DLOG_INFO(MOD_EB, ">>> kvdb test case 3 end");
  }

  void kvdb_test_case_4() {
    DLOG_INFO(MOD_EB, ">>> kvdb test case 4 start");
    cache::KvdbClient::Ptr k1 = cache::KvdbClient::CreateKvdbClient("tkvdb1");
    cache::KvdbClient::Ptr k2 = cache::KvdbClient::CreateKvdbClient("tkvdb2");

    char key[64];
    char w_value[1024];
    vzstd::string r_value;
    int i = 0;

    for(int i = 0; i < 3; i++) {
      sprintf(key, "t1kkk%d", i+1);
      sprintf(w_value, "t1vvv%d", i+1);
      k1->SetKey(key, strlen(key), w_value, strlen(w_value));

      k1->GetKey(key, &r_value);
      if (!strcmp(w_value, (const char*)r_value.c_str())) {
        DLOG_INFO(MOD_EB, "kvdb1 test case 4 successed ^_^");
      } else {
        DLOG_INFO(MOD_EB, "kvdb1 test case 4 failed -_-"
                  ", set value:%s, get value:%s",
                  w_value, r_value.c_str());
      }
    }

    for(int i = 0; i < 3; i++) {
      sprintf(key, "t2kkk%d", i+1);
      sprintf(w_value, "t2vvv%d", i+1);
      k2->SetKey(key, strlen(key), w_value, strlen(w_value));

      k2->GetKey(key, &r_value);
      if (!strcmp(w_value, (const char*)r_value.c_str())) {
        DLOG_INFO(MOD_EB, "kvdb2 test case 4 successed ^_^");
      } else {
        DLOG_INFO(MOD_EB, "kvdb2 test case 4 failed -_-"
                  ", set value:%s, get value:%s",
                  w_value, r_value.c_str());
      }
    }

    if (k1->BackupDatabase()) {
      DLOG_INFO(MOD_EB, "BackupDatabase successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "BackupDatabase failed -_-");
    }
    if (k1->RestoreDatabase()) {
      DLOG_INFO(MOD_EB, "RestoreDatabase successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "RestoreDatabase failed -_-");
    }

    DLOG_INFO(MOD_EB, ">>> kvdb test case 4 end");
  }


  void kvdb_test_case_5() {
    DLOG_INFO(MOD_EB, ">>> kvdb test case 5 start");
    vzstd::string value;
    kvdb_client_->SetProperty(KVDB_SAFE_MODE);
    kvdb_client_->GetKey(KVDB_KEY_WORLD, &value);
    if (!strcmp((const char*)KVDB_VALUE_WORLD, (const char*)value.c_str())) {
      DLOG_INFO(MOD_EB, "kvdb test case 5 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 5 failed -_-, "
                "get value: %s, get value: %s",
                (const char*)KVDB_VALUE_HELLO, value.c_str());
    }
    kvdb_client_->SetKey(KVDB_KEY_WORLD,
                         KVDB_VALUE_WORLD,
                         strlen(KVDB_VALUE_WORLD));
    vzsleep(1000);
    kvdb_client_->GetKey(KVDB_KEY_WORLD, &value);
    if (!strcmp((const char*)KVDB_VALUE_WORLD, (const char*)value.c_str())) {
      DLOG_INFO(MOD_EB, "kvdb test case 5 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 5 failed -_-, "
                "set value: %s, get value: %s",
                (const char*)KVDB_VALUE_HELLO, value.c_str());
    }

    char str[10];
    char tmp[10];
    int x = 1;
    int y;
    memcpy(str, &x, sizeof(int));
    kvdb_client_->SetProperty(KVDB_SAFE_MODE);
    kvdb_client_->GetKey("1test", &y, 4);
    kvdb_client_->GetKey("1test", tmp, 4);
    kvdb_client_->SetKey("1test", str, 4);
    kvdb_client_->GetKey("1test", &y, 4);
    kvdb_client_->GetKey("1test", tmp, 4);

    if (x == y) {
      DLOG_INFO(MOD_EB, "kvdb test case 5 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 5 failed -_-, "
                "set value: %d, get value: %d",
                x, y);
    }

    DLOG_INFO(MOD_EB, ">>> kvdb test case 5 end");
  }

  void kvdb_test_case_6() {
    // C接口测试
    DLOG_INFO(MOD_EB, ">>> kvdb test case 6 start");
    bool successd = false;

    do {
      KVDB_HANDLE kvdb_c = Kvdb_Create("ckvdb6");
      if (NULL == kvdb_c) {
        DLOG_INFO(MOD_EB, "create kvdb instance failed!!!");
        break;
      }

      int result = Kvdb_SetProperty(kvdb_c, KVDB_SAFE_MODE);
      if (!result) {
        DLOG_INFO(MOD_EB, "set SAFE_MODE failed!!!");
        break;
      }

      char buffer[1024] = {0};
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (!result) {
        DLOG_INFO(MOD_EB, "get key failed!!!");
      }

      if (strcmp((const char*)KVDB_VALUE_HELLO, buffer)) {
        DLOG_INFO(MOD_EB, "key value incorrect, "
                  "set value: %s, get value: %s",
                  (const char*)KVDB_VALUE_HELLO, buffer);
      }
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (!result) {
        DLOG_INFO(MOD_EB, "get key failed!!!");
      }

      if (strcmp((const char*)KVDB_VALUE_HELLO, buffer)) {
        DLOG_INFO(MOD_EB, "key value incorrect, "
                  "set value: %s, get value: %s",
                  (const char*)KVDB_VALUE_HELLO, buffer);
      }

      result = Kvdb_SetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           KVDB_VALUE_HELLO, strlen(KVDB_VALUE_HELLO));
      if (!result) {
        DLOG_INFO(MOD_EB, "set key failed!!!");
        break;
      }

      vzsleep(1000);
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (!result) {
        DLOG_INFO(MOD_EB, "get key failed!!!");
        break;
      }

      if (strcmp((const char*)KVDB_VALUE_HELLO, buffer)) {
        DLOG_INFO(MOD_EB, "key value incorrect, "
                  "set value: %s, get value: %s",
                  (const char*)KVDB_VALUE_HELLO, buffer);
        break;
      }
      successd = true;
    } while (0);

    if (successd) {
      DLOG_INFO(MOD_EB, "kvdb test case 6 successed ^_^");
    } else {
      DLOG_INFO(MOD_EB, "kvdb test case 6 failed -_-");
    }
    DLOG_INFO(MOD_EB, ">>> kvdb test case 6 end");
  }

  void kvdb_test_case_7() {
    DLOG_INFO(MOD_EB, ">>> kvdb test case 7 start");

    for (int i=0; i<10; i++) {
      char write_file_name[128] = {0};
#ifdef WIN32
      char *read_file_name = "E:/filecache_test.jpg";
      snprintf(write_file_name, 128, "/kvdb_test_%d.jpg", i+1);
#else
      char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
      snprintf(write_file_name, 128, "/kvdb_test_%d.jpg", i+1);
#endif
      vzes::MemBuffer::Ptr image_buffer;
      image_buffer = cache_client_->Read(read_file_name);
      //DLOG_INFO(MOD_EB, "picture %d: %s", (i+1), write_file_name);
      //vzes::MemBuffer::Ptr image_buffer;
      //image_buffer = cache_client_->Read(read_file_name);
      if (image_buffer && (image_buffer->size() > 0)) {
        char full_path[128] = {0};
        cache_client_->Write(write_file_name, image_buffer->ToString().c_str(),
                             image_buffer->size(), full_path);
        DLOG_INFO(MOD_EB, "save picture %d: %s", (i+1), write_file_name);
      }

      //DLOG_INFO(MOD_EB, "image_buffer reference = %d", image_buffer.use_count());
      vzsleep(100);
    }

    vzsleep(1000);
    cache::CacheClient::DumpCacheInfo();
    cache_client_->ReleaseCache();
    cache::CacheClient::DumpCacheInfo();
    vzes::Block::DumpBlocksInfo();
    DLOG_INFO(MOD_EB, ">>> kvdb test case 7 end");
  }

  void kvdb_test_case_8(){
    kvdb_client_->BackupDatabase();
    printf("please copy file to user_backfolder or wait for 5 min\n");
    printf("input any char to continue\n");
    getchar();
    bool res = kvdb_client_->Clear();
    printf("kvdb_test_case_8 done. please check kvdb.%d\n", res);
  }

  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    kvdb_client_  = cache::KvdbClient::CreateKvdbClient("tkvdb");
    cache_client_ = cache::CacheClient::CreateCacheClient();
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    kvdb_test_case_1();
    kvdb_test_case_2();
    kvdb_test_case_3();
    kvdb_test_case_4();
    kvdb_test_case_5();
    kvdb_test_case_6();
    kvdb_test_case_7();
    kvdb_test_case_8();
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  vzes::EventService::Ptr event_service_;
  cache::CacheClient::Ptr cache_client_;
  cache::KvdbClient::Ptr  kvdb_client_;
};

class FileCacheApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<FileCacheApp>,
  public sigslot::has_slots<> {
 public:
  FileCacheApp() :AppInterface("FileCacheApp") {
  }
  virtual ~FileCacheApp() {
  }

  void filecache_test_case_1() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 1 start");
    char path[256] = {0};

    vzsleep(1000);
#ifdef WIN32
    char *read_file_name = "E:/filecache_test.jpg";
#else
    char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
#endif

    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(read_file_name);
    vzes::BlocksPtr &blocks = read_buffer->blocks();
    vzes::Block::Ptr data_block = blocks.front();
    if (!strncmp((const char*)fc_content, (const char*)data_block->buffer,
                 data_block->buffer_size)) {
      DLOG_INFO(MOD_EB, "filecache test case 1 successed ^_^");
    } else {
      DLOG_ERROR(MOD_EB, "filecache test case 1 failed -_-"
                 ", set value:%s, get value:%s",
                 (const char*)fc_content, data_block->buffer);
    }

    DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer.use_count());
    DLOG_INFO(MOD_EB, ">>> filecache test case 1 end");
  }

  void filecache_test_case_2() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 2 start");

    char full_path_1[128] = {0};
    char full_path_2[128] = {0};
    char full_path_3[128] = {0};
    char full_path_4[128] = {0};
    char full_path_5[128] = {0};
    char full_path_6[128] = {0};
    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content), full_path_1);
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content), full_path_2);
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content), full_path_3);
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content), full_path_4);
    cache_client_->Write(FC_FILE_PATH_5, fc_content, strlen(fc_content), full_path_5);
    cache_client_->Write(FC_FILE_PATH_6, fc_content, strlen(fc_content), full_path_6);

    vzsleep(3000);
    /* 1-3 in cache and flash; 4-5 not in cache and flash; 6 in cache. */
    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(full_path_6);
    if (!read_buffer) {
      DLOG_ERROR(MOD_EB, "filecache test case 2 failed -_-");
    } else {
      read_buffer = cache_client_->Read(full_path_5);
      if (read_buffer) {
        DLOG_ERROR(MOD_EB, "filecache test case 2 failed -_-");
      } else {
        read_buffer = cache_client_->Read(full_path_2);
        if (!read_buffer) {
          DLOG_ERROR(MOD_EB, "filecache test case 2 failed -_-");
        } else {
          DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer.use_count());
          DLOG_INFO(MOD_EB, "filecache test case 2 successed ^_^");
        }
      }
    }

    DLOG_INFO(MOD_EB, ">>> filecache test case 2 end");
  }

  void filecache_test_case_3() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 3 start");

    char full_path_1[128] = {0};
    char full_path_2[128] = {0};
    char full_path_3[128] = {0};
    char full_path_4[128] = {0};
    char full_path_5[128] = {0};
    char full_path_6[128] = {0};
    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content), full_path_1);
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content), full_path_2);
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content), full_path_3);
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content), full_path_4);

    vzsleep(3000); /* make sure file 1-4 have been writen to flash */
    cache_client_->Write(FC_FILE_PATH_5, fc_content, strlen(fc_content), full_path_5);
    cache_client_->Write(FC_FILE_PATH_6, fc_content, strlen(fc_content), full_path_6);

    vzes::MemBuffer::Ptr read_buffer_1;
    read_buffer_1 = cache_client_->Read(full_path_5);
    if (!read_buffer_1) {
      DLOG_ERROR(MOD_EB, "filecache test case 3 failed -_-");
    } else {
      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(full_path_6);
      if (!read_buffer_2) {
        DLOG_ERROR(MOD_EB, "filecache test case 3 failed -_-");
      } else {
        vzes::MemBuffer::Ptr read_buffer_3;
        read_buffer_3 = cache_client_->Read(full_path_5);
        if (!read_buffer_3) {
          DLOG_ERROR(MOD_EB, "filecache test case 3 failed -_-");
        } else {
          DLOG_INFO(MOD_EB, "filecache test case 3 successed ^_^");
        }
      }
    }

    DLOG_INFO(MOD_EB, ">>> filecache test case 3 end");
  }

  void filecache_test_case_4() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 4 start");
    bool successed = true;

    char full_path_1[128] = {0};
    char full_path_2[128] = {0};
    char full_path_3[128] = {0};
    char full_path_4[128] = {0};
    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content), full_path_1);
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content), full_path_2);
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content), full_path_3);
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content), full_path_4);
    vzsleep(3000); /* make sure file 1-4 have been writen to flash */

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(full_path_1);
      DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer_1.use_count());
      if (read_buffer_1.use_count() != 2) {
        DLOG_ERROR(MOD_EB, "filecache test case 4 failed -_-");
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(full_path_1);
      DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer_2.use_count());
      if (read_buffer_2.use_count() != 2) {
        DLOG_ERROR(MOD_EB, "filecache test case 4 failed -_-");
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(full_path_1);
      DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer_1.use_count());
      if (read_buffer_1.use_count() != 2) {
        DLOG_ERROR(MOD_EB, "filecache test case 4 failed -_-");
        successed = false;
      }

      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(full_path_1);
      DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer_2.use_count());
      if (read_buffer_2.use_count() != 3) {
        DLOG_ERROR(MOD_EB, "filecache test case 4 failed -_-");
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(full_path_1);
      DLOG_INFO(MOD_EB, "MemBuffer reference = %d", read_buffer_1.use_count());
      if (read_buffer_1.use_count() != 2) {
        DLOG_ERROR(MOD_EB, "filecache test case 4 failed -_-");
        successed = false;
      }
    }

    if (successed) {
      DLOG_INFO(MOD_EB, "filecache test case 4 successed ^_^");
    }

    DLOG_INFO(MOD_EB, ">>> filecache test case 4 end");
  }

  void filecache_test_case_5() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 5 start");

    for (int i=0; i<10; i++) {
      char write_file_name[128] = {0};
#ifdef WIN32
      char *read_file_name = "E:/filecache_test.jpg";
      snprintf(write_file_name, 128, "filecache_test_%d.jpg", i+1);
#else
      char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
      snprintf(write_file_name, 128, "kvdb/filecache_test_%d.jpg", i+1);
#endif
      vzes::MemBuffer::Ptr image_buffer;
      image_buffer = cache_client_->Read(read_file_name);
      if (image_buffer && (image_buffer->size() > 0)) {
        char full_path[128] = {0};
        cache_client_->Write(write_file_name, image_buffer->ToString().c_str(),
                             image_buffer->size(), full_path);
        DLOG_INFO(MOD_EB, "save picture %d: %s", (i+1), write_file_name);
      }

      //DLOG_INFO(MOD_EB, "image_buffer reference = %d", image_buffer.use_count());
      vzsleep(100);
    }

    vzsleep(1000);
    cache::CacheClient::DumpCacheInfo();
    cache_client_->ReleaseCache();
    cache::CacheClient::DumpCacheInfo();
    vzes::Block::DumpBlocksInfo();
    DLOG_INFO(MOD_EB, ">>> filecache test case 5 end");
  }

  /* filecache���ܲ������� */
  void filecache_test_case_6() {
    vzes::TimeVal tv_start, tv_stop;

    DLOG_INFO(MOD_EB, ">>> filecache test case 6 end");
    char write_file_name[128] = {0};
#ifdef WIN32
    char *read_file_name = "E:/kvdb/filecache_test.jpg";
    snprintf(write_file_name, 128, (char*)"/filecache_test_%d.jpg", 1);
#else
    char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
    snprintf(write_file_name, 128, "/filecache_test_%d.jpg", 1);
#endif

    /* ���Դ�flash��ȡ�ļ�ʱ�� */
    vzes::MemBuffer::Ptr image_buffer;
    vzes::TimeOfDay(&tv_start, NULL);
    image_buffer = cache_client_->Read(read_file_name);
    vzes::TimeOfDay(&tv_stop, NULL);
    printf(">>>> read file from flash, time: %d(usec)\n",
           (tv_stop.sec - tv_start.sec)*1000*1000
           + (tv_stop.usec - tv_start.usec));

    /* ���Դ�cache��ȡ�ļ�ʱ�� */
    vzes::MemBuffer::Ptr image_buffer_2;
    vzes::TimeOfDay(&tv_start, NULL);
    image_buffer_2 = cache_client_->Read(read_file_name);
    vzes::TimeOfDay(&tv_stop, NULL);
    printf(">>>> read file from cache, time - 2: %d(usec)\n",
           (tv_stop.sec - tv_start.sec)*1000*1000
           + (tv_stop.usec - tv_start.usec));

    /* ���Դ�д�ļ�ʱ�� */
    if (image_buffer && (image_buffer->size() > 0)) {
      vzes::TimeOfDay(&tv_start, NULL);
      char full_path[128] = {0};
      cache_client_->Write(write_file_name, image_buffer->ToString().c_str(),
                           image_buffer->size(), full_path);
      vzes::TimeOfDay(&tv_stop, NULL);
      printf(">>>> write file time: %d(usec)\n",
             (tv_stop.sec - tv_start.sec)*1000*1000
             + (tv_stop.usec - tv_start.usec));
    }

    //DLOG_INFO(MOD_EB, "image_buffer reference = %d", image_buffer.use_count());
    image_buffer.reset();
    DLOG_INFO(MOD_EB, ">>> filecache test case 6 end");
  }

  // Base64 test
  void filecache_test_case_7() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 7 start");

    for (int i=0; i<1; i++) {
      char write_file_name[128] = {0};
#ifdef WIN32
      char *read_file_name = "E:/filecache_test.jpg";
      snprintf(write_file_name, 128, "E:/kvdb/filecache_test_%d.jpg", i+1);
#else
      char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
      snprintf(write_file_name, 128, "/mnt/usr/kvdb/filecache_test_%d.jpg", i+1);
#endif
      vzes::MemBuffer::Ptr image_buffer;
      image_buffer = cache_client_->Read(read_file_name);
      if (image_buffer && (image_buffer->size() > 0)) {
        vzstd::string encode_data;
        vzes::BlocksPtr &blocks = image_buffer->blocks();
        vzes::Base64::EncodeFromArray(image_buffer, 0, blocks.size() - 1, &encode_data);
        //vzes::Base64::EncodeFromArray(image_buffer, &encode_data, 0);

        vzstd::string decode_data;
        decode_data = vzes::Base64::Decode(encode_data,  vzes::Base64::DO_LAX);
        Log_HexDump("> decode_data last two bytes",
                    (const unsigned char*)((char*)decode_data.c_str() + decode_data.size() - 2), 2);
        FILE *fp = fopen(write_file_name, "wb");
        if (fp == NULL) {
          DLOG_ERROR(MOD_EB, "failed to open file %s", write_file_name);
          return;
        }
        std::size_t wrs = fwrite(decode_data.c_str(), 1, decode_data.size(), fp);
        if (wrs != decode_data.size()) {
          DLOG_ERROR(MOD_EB, "Write file error:%d", ferror(fp));
          return;
        }
      }

      vzsleep(100);
    }

    vzsleep(1000);
    cache::CacheClient::DumpCacheInfo();
    cache_client_->ReleaseCache();
    cache::CacheClient::DumpCacheInfo();
    vzes::Block::DumpBlocksInfo();
    DLOG_INFO(MOD_EB, ">>> filecache test case 7 end");
  }

  // Base64 test
  void filecache_test_case_8() {
    DLOG_INFO(MOD_EB, ">>> filecache test case 8 start");
    char write_file_name[128] = {0};
#ifdef WIN32
    char *read_file_name = "E:/filecache_test.jpg";
    snprintf(write_file_name, 128, "E:/kvdb/filecache_test_%d.jpg", 1);
#else
    char *read_file_name = (char*)"/mnt/usr/filecache_test.jpg";
    snprintf(write_file_name, 128, "/mnt/usr/kvdb/filecache_test_%d.jpg", 1);
#endif

    char read_buffer[180*1024] = {0};
    FILE *fp = fopen(read_file_name, "rb");
    if (fp == NULL) {
      DLOG_ERROR(MOD_EB, "Failed to open the file %s, err: %d",
                 read_file_name, strerror(errno));
      return;
    }
    int read_size = fread((void *)read_buffer, 1, 180*1024, fp);
    if (read_size <= 0) {
      DLOG_ERROR(MOD_EB, "read file data failed, read_size: %d", read_size);
      return;
    }

    vzstd::string encode_data;
    vzes::Base64::EncodeFromArray((const void*)read_buffer, read_size, &encode_data);

    vzstd::string decode_data;
    decode_data = vzes::Base64::Decode(encode_data,  vzes::Base64::DO_LAX);
    Log_HexDump("> decode_data last two bytes",
                (const unsigned char*)((char*)decode_data.c_str() + decode_data.size() - 2), 2);
    FILE *fp_2 = fopen(write_file_name, "wb");
    if (fp_2 == NULL) {
      DLOG_ERROR(MOD_EB, "failed to open file %s", write_file_name);
      return;
    }
    std::size_t wrs = fwrite(decode_data.c_str(), 1, decode_data.size(), fp_2);
    if (wrs != decode_data.size()) {
      DLOG_ERROR(MOD_EB, "Write file error:%d", ferror(fp));
      return;
    }
    DLOG_INFO(MOD_EB, ">>> filecache test case 8 end");
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    cache_client_ = cache::CacheClient::CreateCacheClient();
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    //filecache_test_case_1();
    //vzsleep(1000);
    //filecache_test_case_2();
    //vzsleep(1000);
    //filecache_test_case_3();
    //vzsleep(1000);
    //filecache_test_case_4();
    filecache_test_case_5();
    //filecache_test_case_6();
    //filecache_test_case_7();
    //filecache_test_case_8();
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  vzes::EventService::Ptr event_service_;
  cache::CacheClient::Ptr cache_client_;
};

int main(void) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr kvdb(new KvdbApp());
  app::AppInterface::Ptr filecache(new FileCacheApp());
  app->RegisterApp(kvdb);
  //app->RegisterApp(filecache);
  app->AppRun();
  while(1);
  app->ExitApp();
  return EXIT_SUCCESS;
}
