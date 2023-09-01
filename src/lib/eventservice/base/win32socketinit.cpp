//

#include "eventservice/base/win32socketinit.h"

#include "eventservice/base/win32.h"

namespace vzes {

// Please don't remove this function.
void EnsureWinsockInit() {
  // The default implementation uses a global initializer, so WSAStartup
  // happens at module load time.  Thus we don't need to do anything here.
  // The hook is provided so that a client that statically links with
  // vzes can override it, to provide its own initialization.
}

#ifdef WIN32
class WinsockInitializer {
 public:
  WinsockInitializer() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1, 0);
    err_ = WSAStartup(wVersionRequested, &wsaData);
  }
  ~WinsockInitializer() {
    if (!err_)
      WSACleanup();
  }
  int error() {
    return err_;
  }
 private:
  int err_;
};
WinsockInitializer g_winsockinit;
#endif

}  // namespace vzes
