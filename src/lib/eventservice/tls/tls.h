//

#ifndef VZBASEEVENT_TLS_TLS_H_  // NOLINT
#define VZBASEEVENT_TLS_TLS_H_

#ifdef WIN32
#include "eventservice/base/win32.h"
#endif

#ifdef POSIX
#include <pthread.h>
#endif

#ifdef WIN32
typedef DWORD TLS_KEY;
#elif LITEOS
typedef void* TLS_KEY;
#elif POSIX
typedef pthread_key_t TLS_KEY;
#endif

namespace vzes {

class Tls {
 public:
  Tls();
  virtual ~Tls();
  void CreateKey(TLS_KEY *key);
  void DeleteKey(TLS_KEY key);
  void SetKey(TLS_KEY key, void *value);
  void *GetKey(TLS_KEY key);
};

}  // namespace vzes

#endif  // VZBASEEVENT_TLS_TLS_H_    // NOLINT
