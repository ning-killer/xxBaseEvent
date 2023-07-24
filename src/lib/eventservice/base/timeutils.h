/*
 * vzes
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


#ifndef SRC_BASE_TIMEUTILS_H_
#define SRC_BASE_TIMEUTILS_H_

#include <time.h>
#include "eventservice/base/common.h"
#ifndef WIN32
#include <sys/time.h>
#endif

#include "eventservice/base/basictypes.h"

namespace vzes {

static const int64 kNumMillisecsPerSec = INT64_C(1000);
static const int64 kNumMicrosecsPerSec = INT64_C(1000000);
static const int64 kNumNanosecsPerSec = INT64_C(1000000000);

static const int64 kNumMicrosecsPerMillisec = kNumMicrosecsPerSec /
    kNumMillisecsPerSec;
static const int64 kNumNanosecsPerMillisec = kNumNanosecsPerSec /
    kNumMillisecsPerSec;

// January 1970, in NTP milliseconds.
static const int64 kJan1970AsNtpMillisecs = INT64_C(2208988800000);

typedef uint32 TimeStamp;

typedef struct {
  uint32 usec;   // 微秒，范围: 0~999999
  uint32 sec;    // 秒，范围: 0~59
  uint32 min;    // 分钟，范围: 0~59
  uint32 hour;   // 小时，范围: 0~23
  uint32 day;    // 日，范围: 1~31
  uint32 month;  // 月，范围: 1~12
  uint32 year;   // 年，范围: 1970~...
  uint32 wday;   // 星期，monday,tuesday,...,范围 : 0 ~6
} TimeLocal;

typedef struct {
  long sec;
  long usec;  // NOLINT
} TimeVal;


// Emulate POSIX gettimeofday(). Gets the current time 
// of the user's timezone.
int TimeOfDay(TimeVal* tv, void *tz);

int GetClockOfDay(struct timeval *tv, void * /*tzv*/);

// Returns the current time in milliseconds.
uint32 Time();
// Returns the current time in nanoseconds.
uint64 TimeNanos();
// Returns the current time in seconds.
uint64 TimeSecond();

// Stores current time in *tm and microseconds in *microseconds.
void CurrentTmTime(struct tm *tm, int *microseconds);

// 返回当前所在时区，东时区为正，西时区为负
int GetLocalTimeZone();

// 和函数TimeMkUTC相反的操作，将秒(UTC时间)转换为当前本地时间，
// 经过时区转换的时间
void TimeMkLocal(TimeLocal *time, uint32 sec);

// 和函数TimeMkLocal相反的操作，将TimeLocal类型的时间日期转换为秒,
// 即转换成从公元1970年1月1日0时0分0 秒算起至今的 UTC 时间所经过的秒数
long TimeMkUTC(TimeLocal time);

// Returns a future timestamp, 'elapsed' milliseconds from now.
uint32 TimeAfter(int32 elapsed);

// Comparisons between time values, which can wrap around.
bool TimeIsBetween(uint32 earlier, uint32 middle, uint32 later);  // Inclusive
bool TimeIsLaterOrEqual(uint32 earlier, uint32 later);  // Inclusive
bool TimeIsLater(uint32 earlier, uint32 later);  // Exclusive

// Returns the later of two timestamps.
inline uint32 TimeMax(uint32 ts1, uint32 ts2) {
  return TimeIsLaterOrEqual(ts1, ts2) ? ts2 : ts1;
}

// Returns the earlier of two timestamps.
inline uint32 TimeMin(uint32 ts1, uint32 ts2) {
  return TimeIsLaterOrEqual(ts1, ts2) ? ts1 : ts2;
}

// Number of milliseconds that would elapse between 'earlier' and 'later'
// timestamps.  The value is negative if 'later' occurs before 'earlier'.
int32 TimeDiff(uint32 later, uint32 earlier);

// The number of milliseconds that have elapsed since 'earlier'.
inline int32 TimeSince(uint32 earlier) {
  return TimeDiff(Time(), earlier);
}

// The number of milliseconds that will elapse between now and 'later'.
inline int32 TimeUntil(uint32 later) {
  return TimeDiff(later, Time());
}

// Converts a unix timestamp in nanoseconds to an NTP timestamp in ms.
inline int64 UnixTimestampNanosecsToNtpMillisecs(int64 unix_ts_ns) {
  return unix_ts_ns / kNumNanosecsPerMillisec + kJan1970AsNtpMillisecs;
}

}  // namespace vzes

#endif  // SRC_BASE_TIMEUTILS_H_
