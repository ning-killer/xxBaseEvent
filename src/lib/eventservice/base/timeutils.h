//


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
  uint32 usec;   // ΢�룬��Χ: 0~999999
  uint32 sec;    // �룬��Χ: 0~59
  uint32 min;    // ���ӣ���Χ: 0~59
  uint32 hour;   // Сʱ����Χ: 0~23
  uint32 day;    // �գ���Χ: 1~31
  uint32 month;  // �£���Χ: 1~12
  uint32 year;   // �꣬��Χ: 1970~...
  uint32 wday;   // ���ڣ�monday,tuesday,...,��Χ : 0 ~6
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

// ���ص�ǰ����ʱ������ʱ��Ϊ������ʱ��Ϊ��
int GetLocalTimeZone();

// �ͺ���TimeMkUTC�෴�Ĳ���������(UTCʱ��)ת��Ϊ��ǰ����ʱ�䣬
// ����ʱ��ת����ʱ��
void TimeMkLocal(TimeLocal *time, uint32 sec);

// �ͺ���TimeMkLocal�෴�Ĳ�������TimeLocal���͵�ʱ������ת��Ϊ��,
// ��ת���ɴӹ�Ԫ1970��1��1��0ʱ0��0 ����������� UTC ʱ��������������
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
