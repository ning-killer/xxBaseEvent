//

#ifndef EVENTSERVICES_BASE_NETHELPERS_H_
#define EVENTSERVICES_BASE_NETHELPERS_H_

#ifdef POSIX
#include <netdb.h>
#include <cstddef>
#elif WIN32
#include <winsock2.h>  // NOLINT
#endif

#include <list>

#include "eventservice/base/signalthread.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/base/socketaddress.h"

namespace vzes {

// AsyncResolver will perform async DNS resolution, signaling the result on
// the inherited SignalWorkDone when the operation completes.
class AsyncResolver : public SignalThread {
 public:
  AsyncResolver();

  const SocketAddress& address() const {
    return addr_;
  }
  const std::vector<IPAddress>& addresses() const {
    return addresses_;
  }
  void set_address(const SocketAddress& addr) {
    addr_ = addr;
  }
  int error() const {
    return error_;
  }
  void set_error(int error) {
    error_ = error;
  }


 protected:
  virtual void DoWork();
  virtual void OnWorkDone();

 private:
  SocketAddress addr_;
  std::vector<IPAddress> addresses_;
  int error_;
};

// vzes namespaced wrappers for inet_ntop and inet_pton so we can avoid
// the windows-native versions of these.
const char* inet_ntop(int af, const void *src, char* dst, socklen_t size);
int inet_pton(int af, const char* src, void *dst);

bool HasIPv6Enabled();
}  // namespace vzes

#endif  // EVENTSERVICES_BASE_NETHELPERS_H_
