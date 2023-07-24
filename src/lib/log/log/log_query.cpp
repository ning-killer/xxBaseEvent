
#include "stdio.h"
#include <string.h>
#include <algorithm>
#include "eventservice/base/timeutils.h"
#include "log/log/log_client.h"
#include "log/log/log_server.h"
#include "eventservice/base/common.h"

typedef struct {
  int file_idx;         //打开的文件(0代表A, 1代表B)
  bool is_first_file;
  FILE *fpidx[2], *fpfile[2];

  int cur_index;        //索引的下标
  int max_index;        //当前文件索引下标最大值
  int file_offset;      //索引指向文件offset

  bool hasres;          //此类文件是否还有结果
  bool isopened;        //此类文件是否打开了文件
  LOG_NODE_FILE_S data;
} LOG_FILE_QUERY_S;


typedef uint32 LOG_INDEX_VALUE;
#define LOG_TYPE_E_MAX_SIZE (5)
static bool useCustomFolder = false;
static char customFolder[LOG_FILFPATH_MAXLEN];

//a < b return -1
//a = b return 0
//a > b return 1
static int log_CompareTimeS(const LOG_TIME_S &a, const LOG_TIME_S &b) {
  if (a.sec < b.sec) {
    return -1;
  } else if (a.sec > b.sec) {
    return 1;
  } else if (a.usec < b.usec) {
    return -1;
  } else if (a.usec > b.usec) {
    return 1;
  }
  return 0;
}

static bool log_LoadFileFirstRecordDesc(LOG_FILE_QUERY_S &cur,
                                       const uint32 st_id,
                                       const uint32 ed_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false; //无数据
  }

  int len = offset / sizeof(LOG_INDEX_VALUE);
  int tmp_len = len;

  int fi = len-1, half = 0;
  LOG_INDEX_VALUE tmp_idx;
  LOG_NODE_FILE_S tmp_lnfs;
  while (len) {
    half = len >> 1;

    fseek(fpidx, (fi - half) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
      DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
      return false;
    }
    fseek(fpfile, tmp_idx, SEEK_SET);
    if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp_lnfs.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    if (tmp_lnfs.id > ed_id) {
      len = len - half - 1;
      fi = fi - half - 1;
    } else {
      len = half;
    }
  }
  if (fi == -1) {
    return false;//无结果
  }
  //有结果
  fseek(fpidx, (fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
  if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
    DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
    return false;
  }
  fseek(fpfile, tmp_idx, SEEK_SET);
  if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
    DLOG_WARNING(MOD_EB, "fread error: read data error\n");
    return false;
  } else if (tmp_lnfs.head != 0x47) {
    DLOG_WARNING(MOD_EB, "read data head error\n");
    return false;
  }
  if (tmp_lnfs.id < st_id) {
    //第一条记录不在范围内
    return false;
  }

  cur.cur_index = fi;
  cur.data = tmp_lnfs;
  cur.file_offset = tmp_idx;
  cur.max_index = tmp_len;
  return true;
}

static bool log_LoadFileFirstRecordAsc(LOG_FILE_QUERY_S &cur,
                                    const uint32 st_id,
                                    const uint32 ed_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false; //无数据
  }

  int len = offset / sizeof(LOG_INDEX_VALUE);
  int tmp_len = len;

  int fi = 0, half = 0;
  LOG_INDEX_VALUE tmp_idx;
  LOG_NODE_FILE_S tmp_lnfs;
  while (len) {
    half = len >> 1;

    fseek(fpidx, (half + fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
      DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
      return false;
    }
    fseek(fpfile, tmp_idx, SEEK_SET);
    if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp_lnfs.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    if (tmp_lnfs.id < st_id) {
      len = len - half - 1;
      fi = fi + half + 1;
    } else {
      len = half;
    }
  }
  if (fi == tmp_len) {
    return false;//无结果
  }
  //有结果
  fseek(fpidx, (fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
  if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
    DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
    return false;
  }
  fseek(fpfile, tmp_idx, SEEK_SET);
  if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
    DLOG_WARNING(MOD_EB, "fread error: read data error\n");
    return false;
  } else if (tmp_lnfs.head != 0x47) {
    DLOG_WARNING(MOD_EB, "read data head error\n");
    return false;
  }
  if (tmp_lnfs.id > ed_id) {
    //第一条记录不在范围内
    return false;
  }

  cur.cur_index = fi;
  cur.data = tmp_lnfs;
  cur.file_offset = tmp_idx;
  cur.max_index = tmp_len;
  return true;
}


static bool log_FindCategoryFirstRecord(LOG_FILE_QUERY_S &cur,
                                        const uint32 min_id,
                                        const uint32 max_id,
                                        LOG_QUERY_TYPE qtype) {
  cur.is_first_file = true;
  LOG_FILE_QUERY_S fa, fb;
  fa = cur;
  fa.file_idx = 0;
  fb = cur;
  fb.file_idx = 1;
  int resa, resb;
  if(qtype == LOG_QUERY_ASC_TYPE){
    resa = log_LoadFileFirstRecordAsc(fa, min_id, max_id);
    resb = log_LoadFileFirstRecordAsc(fb, min_id, max_id);
  } else {
    resa = log_LoadFileFirstRecordDesc(fa, min_id, max_id);
    resb = log_LoadFileFirstRecordDesc(fb, min_id, max_id);
  }
  if (resa && resb) {
    if(qtype == LOG_QUERY_ASC_TYPE){
      if (fa.data.id < fb.data.id) {
        cur = fa;
      } else {
        cur = fb;
      }
    } else {
      if (fa.data.id > fb.data.id) {
        cur = fa;
      } else {
        cur = fb;
      }
    }
    return true;
  } else if (resa) {
    cur = fa;
    return true;
  } else if (resb) {
    cur = fb;
    return true;
  }
  return false;
}

static bool log_FindFileIdRange(LOG_FILE_QUERY_S &cur,
                                const LOG_TIME_S &st,
                                const LOG_TIME_S &ed,
                                uint32 &min_id,
                                uint32 &max_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false; //无数据
  }
  min_id = ~0;
  max_id = 0;

  int len = offset / sizeof(LOG_INDEX_VALUE);
  LOG_NODE_FILE_S tmp;
  bool find_record = false;
  for (int i = 0; i < len; i++) {
    fseek(fpidx, i * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    int offset;
    if (fread(&offset, sizeof(LOG_INDEX_VALUE), 1, fpidx) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read index error\n");
      return false;
    }
    fseek(fpfile, offset, SEEK_SET);
    if (fread(&tmp, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    LOG_TIME_S t;
    t.sec = tmp.node.sec;
    t.usec = tmp.node.usec;
    if (log_CompareTimeS(st, t) <= 0 &&
        log_CompareTimeS(t, ed) <= 0) {
      min_id = VZ_MIN(min_id, tmp.id);
      max_id = VZ_MAX(max_id, tmp.id);
      find_record = true;
    }
  }
  if (find_record) {
    return true;
  } else {
    return false;
  }
}

static bool log_FindCategoryIdRange(LOG_FILE_QUERY_S &cur,
                                    const LOG_TIME_S st,
                                    const LOG_TIME_S ed,
                                    uint32 &min_id,
                                    uint32 &max_id) {
  LOG_FILE_QUERY_S fa, fb;
  fa = cur;
  fa.file_idx = 0;
  fb = cur;
  fb.file_idx = 1;
  uint32 max_id_a, max_id_b;
  uint32 min_id_a, min_id_b;
  bool resa = log_FindFileIdRange(fa, st, ed, min_id_a, max_id_a);
  bool resb = log_FindFileIdRange(fb, st, ed, min_id_b, max_id_b);
  if (resa && resb) {
    min_id = VZ_MIN(min_id_a, min_id_b);
    max_id = VZ_MAX(max_id_a, max_id_b);
    return true;
  } else if (resa) {
    min_id = min_id_a;
    max_id = max_id_a;
    return true;
  } else if (resb) {
    min_id = min_id_b;
    max_id = max_id_b;
    return true;
  }
  return false;
}

static int log_LoadNextRecord(LOG_FILE_QUERY_S &cur,
                              const uint32 min_id,
                              const uint32 max_id,
                              LOG_QUERY_TYPE qtype) {
  if(qtype == LOG_QUERY_ASC_TYPE){
    cur.cur_index++;
  } else{
    cur.cur_index--;
  }
  if(cur.cur_index >= 0 && cur.cur_index < cur.max_index) {
    //当前文件未读到末尾
    FILE *fpfile = cur.fpfile[cur.file_idx];
    FILE *fpidx = cur.fpidx[cur.file_idx];

    fseek(fpidx, cur.cur_index * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (fread(&cur.file_offset, sizeof(LOG_INDEX_VALUE), 1, fpidx) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read index error\n");
      return 0;
    }
    fseek(fpfile, cur.file_offset, SEEK_SET);
    if (fread(&cur.data, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return 0;
    } else if (cur.data.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head is error\n");
      return 0;
    }
    if (cur.data.id >= min_id && cur.data.id <= max_id) {
      return 1;
    } else {
      return 0;
    }
  } else {
    //当前文件已读到末尾
    if(!cur.is_first_file){
      return 0;
    }
    cur.file_idx = 1 - cur.file_idx;
    cur.is_first_file = false;

    FILE *fpfile = cur.fpfile[cur.file_idx];
    FILE *fpidx = cur.fpidx[cur.file_idx];
    fseek(fpidx, 0, SEEK_END);
    cur.max_index = ftell(fpidx) / sizeof(LOG_INDEX_VALUE);
    if (cur.max_index == 0) {
      //下一个文件无信息
      return 0;
    } else {
      //下一个文件有信息
      if(qtype== LOG_QUERY_ASC_TYPE){
        cur.cur_index = -1;
      } else {
        cur.cur_index = cur.max_index;
      }
      return log_LoadNextRecord(cur, min_id, max_id, qtype);
    }
  }
  return 0;
}

static bool log_openCategoryFiles(LOG_FILE_QUERY_S &cur,
                                  char *filepath[]) {
  for (int i = 0; i < 4; i++) {
    FILE **fpp;
    if (i == 0) {
      fpp = &cur.fpfile[0];
    } else if(i == 1) {
      fpp = &cur.fpidx[0];
    } else if (i == 2) {
      fpp = &cur.fpfile[1];
    } else {
      fpp = &cur.fpidx[1];
    }
    *fpp = fopen(filepath[i], "rb");
    if (*fpp == NULL) {
      DLOG_WARNING(MOD_EB, "open file %s error\n", filepath[i]);
      *fpp = fopen(filepath[i], "wb+");
      if (*fpp == NULL) {
        DLOG_WARNING(MOD_EB, "create file %s error\n", filepath[i]);
        return false;
      }
    }
  }
  return true;
}

bool Log_SearchUseCustomFolder(const char folder[128], int len) {
  useCustomFolder = true;
  if (len >= LOG_FOlDPATH_MAXLEN) {
    len = LOG_FOlDPATH_MAXLEN - 1;
  }
  memcpy(customFolder, folder, len);
  customFolder[len] = '\0';
  return true;
}

static void log_GetCustomFileName(LOG_SRV_FILE_TYPE_E fid,
                                  int index,
                                  char *out_data_name,
                                  char *out_index_name,
                                  int maxlen) {
  const char *foldpath;
#if defined(WIN32)
  const char *sformat_data[] = {
    "%s\\debug%d.log", "%s\\point%d.log", "%s\\sys%d.log", "%s\\dev%d.log", "%s\\ui%d.log"
  };
  const char *sformat_index[] = {
    "%s\\debug%d_id.log", "%s\\point%d_id.log", "%s\\sys%d_id.log", "%s\\dev%d_id.log", "%s\\ui%d_id.log"
  };
#else
  const char *sformat_data[] = {
    "%s/debug%d.log", "%s/point%d.log", "%s/sys%d.log", "%s/dev%d.log", "%s/ui%d.log"
  };
  const char *sformat_index[] = {
    "%s/debug%d_id.log", "%s/point%d_id.log", "%s/sys%d_id.log", "%s/dev%d_id.log", "%s/ui%d_id.log"
  };
#endif
  foldpath = customFolder;

  snprintf(out_data_name, maxlen, sformat_data[fid], foldpath, index);
  snprintf(out_index_name, maxlen, sformat_index[fid], foldpath, index);
}

static bool log_CloseAllFiles(LOG_FILE_QUERY_S arr[], int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i].fpfile[0] != NULL) {
      fclose(arr[i].fpfile[0]);
    }
    if (arr[i].fpfile[1] != NULL) {
      fclose(arr[i].fpfile[1]);
    }
    if (arr[i].fpidx[0] != NULL) {
      fclose(arr[i].fpidx[0]);
    }
    if (arr[i].fpidx[1] != NULL) {
      fclose(arr[i].fpidx[1]);
    }
  }
  return true;
}

static int log_OpenTypeFiles(LOG_FILE_QUERY_S arr[], int size,
                             const uint8 type_mask) {
  char file_name[4][LOG_FILFPATH_MAXLEN];
  char *file_name_array[4] = {
    file_name[0], file_name[1], file_name[2], file_name[3]
  };
  for (int i = 0; i < size; i++) {
    arr[i].fpfile[0] = arr[i].fpfile[1] = NULL;
    arr[i].fpidx[0] = arr[i].fpidx[1] = NULL;
  }
  for (int i = 0; i < size; i++) {
    uint8 cur_type;
    if (i == 0) {
      cur_type = LT_DEBUG;
    } else if (i == 1) {
      cur_type = LT_POINT;
    } else if (i == 2) {
      cur_type = LT_SYS;
    } else if (i == 3) {
      cur_type = LT_DEV;
    } else if (i == 4) {
      cur_type = LT_UI;
    } else {
      cur_type = 0;
    }
    arr[i].hasres = false;
    arr[i].isopened = false;
    if (!(type_mask & cur_type)) {
      //未查询当前type;
      continue;
    }
    LOG_SRV_FILE_TYPE_E ftype;
    if (!Log_SrvGetFileType((LOG_TYPE_E)cur_type, &ftype)) {
      continue;
    }
    if (useCustomFolder) {
      log_GetCustomFileName(ftype, 0,
                            file_name[0], file_name[1],
                            LOG_FILFPATH_MAXLEN);
      log_GetCustomFileName(ftype, 1,
                            file_name[2], file_name[3],
                            LOG_FILFPATH_MAXLEN);
    } else {
      Log_SrvGetFileName(ftype, 0,
                         file_name[0], file_name[1],
                         LOG_FILFPATH_MAXLEN);
      Log_SrvGetFileName(ftype, 1,
                         file_name[2], file_name[3],
                         LOG_FILFPATH_MAXLEN);
    }
    arr[i].isopened = true;
    if (!log_openCategoryFiles(arr[i], file_name_array)) {
      //打开文件失败 异常，退出。
      log_CloseAllFiles(arr, size);
      return false;
    }
  }
  return true;
}

static bool log_GetIdRange(LOG_FILE_QUERY_S arr[], int size,
                           const LOG_TIME_S start,
                           const LOG_TIME_S end,
                           LOG_QUERY_NODE &qnode) {
  if (!qnode.is_first) {
    //非第一次访问，使用用户传入id的范围缓存，不需要计算。
    return true;
  }
  qnode.min_id = ~0;
  qnode.max_id = 0;
  bool hasres = false;
  for (int i = 0; i < size; i++) {
    uint32 tmp_min_id, tmp_max_id;
    if (!arr[i].isopened) {
      continue;
    }
    if (log_FindCategoryIdRange(arr[i], start, end, tmp_min_id, tmp_max_id)) {
      qnode.max_id = VZ_MAX(qnode.max_id, tmp_max_id);
      qnode.min_id = VZ_MIN(qnode.min_id, tmp_min_id);
      hasres = true;
    }
  }
  return hasres;
}

int Log_Search(const uint8 type_mask,
               const LOG_TIME_S start,
               const LOG_TIME_S end,
               const uint32 size,
               char buffer[][LOG_RELEASE_MAX_LEN],
               LOG_QUERY_NODE &qnode) {

  LOG_FILE_QUERY_S arr[LOG_TYPE_E_MAX_SIZE];
  int res_type_count = 0;

  vzes::TimeLocal start_time, end_time;
  vzes::TimeMkLocal(&start_time, start.sec);
  vzes::TimeMkLocal(&end_time, end.sec);

  DLOG_INFO(MOD_EB, "Log search req,start:%4d%02d%02d %02d:%02d:%02d,"
            "end:%4d%02d%02d %02d:%02d:%02d,count:%d",
            start_time.year, start_time.month, start_time.day,
            start_time.hour, start_time.min, start_time.sec,
            end_time.year, end_time.month, end_time.day,
            end_time.hour, end_time.min, end_time.sec, size);

  //加载文件
  if (!useCustomFolder) {
    LOGSVR_FILE_LOCK;
  }
  if (!log_OpenTypeFiles(arr, LOG_TYPE_E_MAX_SIZE, type_mask)) {
    if (!useCustomFolder) {
      LOGSVR_FILE_UNLOCK;
    }
    return 0;
  }
  //获取id范围
  if (!log_GetIdRange(arr, LOG_TYPE_E_MAX_SIZE, start, end, qnode)) {
    log_CloseAllFiles(arr, LOG_TYPE_E_MAX_SIZE);
    if (!useCustomFolder) {
      LOGSVR_FILE_UNLOCK;
    }
    return 0;
  }
  //加载文件对应数据
  uint32 cur_min, cur_max;
  if(qnode.is_first){
    if(qnode.qtype == LOG_QUERY_ASC_TYPE){
      qnode.start_id = 0;
    } else {
      qnode.start_id = ~(uint32)0;
    }
  }
  if(qnode.qtype == LOG_QUERY_ASC_TYPE){
    cur_min = VZ_MAX(qnode.min_id, qnode.start_id+1);
    cur_max = qnode.max_id;
  } else {
    cur_min = qnode.min_id;
    cur_max = VZ_MIN(qnode.max_id, qnode.start_id-1);
  }
  for (int i = 0; i < LOG_TYPE_E_MAX_SIZE; i++) {
    if (arr[i].isopened) {
      if (log_FindCategoryFirstRecord(arr[i], cur_min, cur_max, qnode.qtype)) {
        arr[i].hasres = true;
        res_type_count++;
      }
    }
  }

  //多路归并并输出到Buff
  uint32 added = 0;
  while (added < size && res_type_count != 0) {
    //找最小的id
    int idx;
    uint32 id;
    bool update_id = false;
    for (int i = 0; i < LOG_TYPE_E_MAX_SIZE; i++) {
      if (!arr[i].hasres) {
        continue;
      }
      uint32 tmpid = arr[i].data.id;
      if(!update_id){
        update_id = true;
        idx = i;
        id = tmpid;
        continue;
      }
      if(qnode.qtype == LOG_QUERY_ASC_TYPE){
        if (tmpid < id) {
          idx = i;
          id = tmpid;
        }
      } else{
        if (tmpid > id) {
          idx = i;
          id = tmpid;
        }
      }
    }

    //写buff
    if (arr[idx].data.node.length <= LOG_RELEASE_MAX_LEN) {
      FILE *fpfile = arr[idx].fpfile[arr[idx].file_idx];
      fseek(fpfile, arr[idx].file_offset + 5, SEEK_SET);
      if (fread(buffer[added], arr[idx].data.node.length, 1, fpfile) != 1) {
        DLOG_WARNING(MOD_EB, "fread data to buffer error\n");
        break;
      }
      added++;
      qnode.last_id = arr[idx].data.id;
    }
    //获取下一个
    if (!log_LoadNextRecord(arr[idx], cur_min, cur_max, qnode.qtype)) {
      arr[idx].hasres = false;
      res_type_count--;
    }
  }

  //关闭文件
  log_CloseAllFiles(arr, LOG_TYPE_E_MAX_SIZE);
  if (!useCustomFolder) {
    LOGSVR_FILE_UNLOCK;
  }
  DLOG_INFO(MOD_EB, "Log search done, find count:%d", added);
  return added;
}