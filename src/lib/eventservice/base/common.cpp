//

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#if WIN32
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif  // WIN32

#if OSX
#include <CoreServices/CoreServices.h>
#endif  // OSX

#include <algorithm>
#include "eventservice/base/common.h"
#include "log/log/log_client.h"

//////////////////////////////////////////////////////////////////////
// Assertions
//////////////////////////////////////////////////////////////////////

namespace vzes {

void Break() {
#ifdef WIN32
  ::DebugBreak();
#elif defined(LITEOS)
  // On LITEOS system not support SIGTRAP,temporarily block this feature.
#else
  // On POSIX systems, SIGTRAP signals debuggers to break without killing the
  // process. If a debugger isn't attached, the uncaught SIGTRAP will crash the
  // app.
  raise(SIGTRAP);
#endif
  // If a debugger wasn't attached, we will have crashed by this point. If a
  // debugger is attached, we'll continue from here.
}

static AssertLogger custom_assert_logger_ = NULL;

void SetCustomAssertLogger(AssertLogger logger) {
  custom_assert_logger_ = logger;
}

void LogAssert(const char* function, const char* file, int line,
               const char* expression) {
  if (custom_assert_logger_) {
    custom_assert_logger_(function, file, line, expression);
  } else {
    DLOG_ERROR(MOD_EB, "%s(%d): ASSERT FAILED: %s @ ",
               file, line, expression, function);
  }
}

}  // namespace vzes
