//

#ifdef POSIX
#include <sys/time.h>
#if defined(OSX) || defined(IOS)
#include <mach/clock.h>
#include <mach/mach_time.h>
#endif
#endif

#ifdef WIN32
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

#include "eventservice/base/common.h"
#include "eventservice/base/timeutils.h"

#define EFFICIENT_IMPLEMENTATION 1

namespace vzes {

const uint32 LAST = 0xFFFFFFFF;
const uint32 HALF = 0x80000000;

uint64 TimeNanos() {
  int64 ticks = 0;
#if defined(OSX) || defined(IOS)
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0) {
    // Get the timebase if this is the first time we run.
    // Recommended by Apple's QA1398.
    VZ_VERIFY(KERN_SUCCESS == mach_timebase_info(&timebase));
  }
  // Use timebase to convert absolute time tick units into nanoseconds.
  ticks = mach_absolute_time() * timebase.numer / timebase.denom;
#elif defined(POSIX)
  struct timespec ts;
  // Do we need to handle the case when CLOCK_MONOTONIC
  // is not supported?
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ticks = kNumNanosecsPerSec * static_cast<int64>(ts.tv_sec) +
          static_cast<int64>(ts.tv_nsec);
#elif defined(WIN32)
  static volatile LONG last_timegettime = 0;
  static volatile int64 num_wrap_timegettime = 0;
  volatile LONG* last_timegettime_ptr = &last_timegettime;
  DWORD now = timeGetTime();
  // Atomically update the last gotten time
  DWORD old = InterlockedExchange(last_timegettime_ptr, now);
  if (now < old) {
    // If now is earlier than old, there may have been a race between
    // threads.
    // 0x0fffffff ~3.1 days, the code will not take that long to execute
    // so it must have been a wrap around.
    if (old > 0xf0000000 && now < 0x0fffffff) {
      num_wrap_timegettime++;
    }
  }
  ticks = now + (num_wrap_timegettime << 32);
  // Calculate with nanosecond precision.  Otherwise, we're just
  // wasting a multiply and divide when doing Time() on Windows.
  ticks = ticks * kNumNanosecsPerMillisec;
#endif
  return ticks;
}

uint64 TimeSecond() {
  return static_cast<uint64>(TimeNanos() / kNumNanosecsPerSec);
}

uint32 Time() {
  return static_cast<uint32>(TimeNanos() / kNumNanosecsPerMillisec);
}

int TimeOfDay(TimeVal* tv, void *tz) {
#if defined(WIN32)
  static const uint64 kFileTimeToUnixTimeEpochOffset = 116444736000000000ULL;

  // FILETIME is measured in tens of microseconds since 1601-01-01 UTC.
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  LARGE_INTEGER li;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;

  // ��õ�ǰwindows������ʱ����ʱ��������Эͬ������ʵʱ�����ʱ�����ʱ���
  int time_zone = GetLocalTimeZone();
  long time_zone_diff = time_zone * 3600;

  // Convert to seconds and microseconds since Unix time Epoch.
  int64 micros = (li.QuadPart - kFileTimeToUnixTimeEpochOffset) / 10;
  tv->sec = static_cast<long>(micros / kNumMicrosecsPerSec) + time_zone_diff;  // NOLINT
  tv->usec = static_cast<long>(micros % kNumMicrosecsPerSec); // NOLINT
#else
  struct timeval time_local;
  gettimeofday(&time_local, NULL);
  tv->sec = time_local.tv_sec;
  tv->usec = time_local.tv_usec;
#endif
  return 0;
}

int GetClockOfDay(struct timeval *tv, void * /*tzv*/) {
#if defined(POSIX)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (tv) {
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
    return 0;
  }
  return -1;
#else defined(WIN32)
  tv = NULL;
  return -1;
#endif
}

#if defined(WIN32)
// Emulate POSIX gmtime_r().
// ���ص�ʱ������δ��ʱ��ת������UTCʱ��(�ֳ�Ϊ����ʱ�䣬����������ʱ��)
static struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *tm = NULL;
#ifdef WIN32
  // On Windows, gmtime is thread safe.
  struct tm t_tm;
  gmtime_s(&t_tm, timep);
  tm = &t_tm;
#else
  tm = gmtime(timep);  // NOLINT
#endif
  if (tm == NULL) {
    return NULL;
  }
  *result = *tm;
  return result;
}
#endif  // WIN32

void CurrentTmTime(struct tm *tm, int *microseconds) {
  TimeVal timeval;
  if (TimeOfDay(&timeval, NULL) < 0) {
    // Incredibly unlikely code path.
    timeval.sec = timeval.usec = 0;
  }
  time_t secs = timeval.sec;
  gmtime_r(&secs, tm);
  *microseconds = timeval.usec;
}

// ���ص�ǰwindows����ʱ��������Ƕ�ʱ��Ϊ�����������ʱ��Ϊ����
int GetLocalTimeZone() {
#ifdef WIN32
  time_t time_utc;
  struct tm tm_local, tm_gmt;
  time(&time_utc);
  localtime_s(&tm_local, &time_utc);
  gmtime_s(&tm_gmt, &time_utc);

  int time_zone = tm_local.tm_hour - tm_gmt.tm_hour;
  if (time_zone < -12) {
    time_zone = time_zone + 24;
  } else if (time_zone > 12) {
    time_zone = time_zone - 24;
  }
  return time_zone;
#endif
  return 0;
}

void TimeMkLocal(TimeLocal *time, uint32 sec) {
#ifdef WIN32
  // windows��ʱ�����C���Эͬ��ת����Ҫ����0ʱ��������
  int time_zone = 0;
  time_zone = GetLocalTimeZone();
  int time_zone_diff = time_zone * 3600;
  sec = sec - time_zone_diff;
#endif
  // ��ȡ����ʱ��,������ʱ��ת����ʱ��
  time_t secs = sec;
  struct tm *tp = localtime(&secs);

  time->usec  = 0;
  time->sec   = (uint32)tp->tm_sec;
  time->min   = (uint32)tp->tm_min;
  time->hour  = (uint32)tp->tm_hour;
  time->day   = (uint32)tp->tm_mday;
  time->month = (uint32)tp->tm_mon + 1U;
  time->year  = (uint32)tp->tm_year + 1900U;
  time->wday  = (uint32)tp->tm_wday;
}

long TimeMkUTC(TimeLocal time) {
  struct tm tp = { 0 };

  tp.tm_sec	  = time.sec;
  tp.tm_min	  = time.min;
  tp.tm_hour  = time.hour;
  tp.tm_mday  = time.day;
  tp.tm_mon	  = time.month - 1;
  tp.tm_year  = time.year - 1900;
  tp.tm_wday  = time.wday;
  tp.tm_isdst = -1;
  return (long)mktime(&tp);
}

uint32 TimeAfter(int32 elapsed) {
  ASSERT(elapsed >= 0);
  ASSERT(static_cast<uint32>(elapsed) < HALF);
  return Time() + elapsed;
}

bool TimeIsBetween(uint32 earlier, uint32 middle, uint32 later) {
  if (earlier <= later) {
    return ((earlier <= middle) && (middle <= later));
  } else {
    return !((later < middle) && (middle < earlier));
  }
}

bool TimeIsLaterOrEqual(uint32 earlier, uint32 later) {
#if EFFICIENT_IMPLEMENTATION
  int32 diff = later - earlier;
  return (diff >= 0 && static_cast<uint32>(diff) < HALF);
#else
  const bool later_or_equal = TimeIsBetween(earlier, later, earlier + HALF);
  return later_or_equal;
#endif
}

bool TimeIsLater(uint32 earlier, uint32 later) {
#if EFFICIENT_IMPLEMENTATION
  int32 diff = later - earlier;
  return (diff > 0 && static_cast<uint32>(diff) < HALF);
#else
  const bool earlier_or_equal = TimeIsBetween(later, earlier, later + HALF);
  return !earlier_or_equal;
#endif
}

int32 TimeDiff(uint32 later, uint32 earlier) {
#if EFFICIENT_IMPLEMENTATION
  return later - earlier;
#else
  const bool later_or_equal = TimeIsBetween(earlier, later, earlier + HALF);
  if (later_or_equal) {
    if (earlier <= later) {
      return static_cast<int32>(later - earlier);
    } else {
      return static_cast<int32>(later + (LAST - earlier) + 1);
    }
  } else {
    if (later <= earlier) {
      return -static_cast<int32>(earlier - later);
    } else {
      return -static_cast<int32>(earlier + (LAST - later) + 1);
    }
  }
#endif
}

}  // namespace vzes
