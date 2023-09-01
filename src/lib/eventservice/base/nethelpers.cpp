//


#include "eventservice/base/nethelpers.h"

#if defined(WIN32)
#include <ws2spi.h>
#include <ws2tcpip.h>
#include "eventservice/base/win32.h"
#endif

#include "eventservice/base/byteorder.h"
#include "eventservice/base/signalthread.h"

namespace vzes {

int ResolveHostname(const std::string& hostname, int family,
                    std::vector<IPAddress>* addresses) {
  if (!addresses) {
    return -1;
  }
  //addresses->clear();
  //struct addrinfo* result = NULL;
  //struct addrinfo hints = {0};
  //// TODO(djw): For now this is IPv4 only so existing users remain unaffected.
  //hints.ai_family = AF_INET;
  //hints.ai_flags = AI_ADDRCONFIG;
  //int ret = getaddrinfo(hostname.c_str(), NULL, &hints, &result);
  //if (ret != 0) {
  //  return ret;
  //}
  //struct addrinfo* cursor = result;
  //for (; cursor; cursor = cursor->ai_next) {
  //  if (family == AF_UNSPEC || cursor->ai_family == family) {
  //    IPAddress ip;
  //    if (IPFromAddrInfo(cursor, &ip)) {
  //      addresses->push_back(ip);
  //    }
  //  }
  //}
  //freeaddrinfo(result);
  return 0;
}

// AsyncResolver
AsyncResolver::AsyncResolver() : error_(0) {
}

void AsyncResolver::DoWork() {
  error_ = ResolveHostname(addr_.hostname().c_str(), addr_.family(),
                           &addresses_);
}

void AsyncResolver::OnWorkDone() {
  if (addresses_.size() > 0) {
    addr_.SetIP(addresses_[0]);
  }
}

const char* inet_ntop(int af, const void *src, char* dst, socklen_t size) {
#ifdef WIN32
  return win32_inet_ntop(af, src, dst, size);
#else
  return ::inet_ntop(af, src, dst, size);
#endif
}

int inet_pton(int af, const char* src, void *dst) {
#ifdef WIN32
  return win32_inet_pton(af, src, dst);
#else
  return ::inet_pton(af, src, dst);
#endif
}

bool HasIPv6Enabled() {
#ifndef WIN32
  // We only need to check this for Windows XP (so far).
  return true;
#else
  if (IsWindowsVistaOrLater()) {
    return true;
  }
  if (!IsWindowsXpOrLater()) {
    return false;
  }
  DWORD protbuff_size = 4096;
  scoped_array<char> protocols;
  LPWSAPROTOCOL_INFOW protocol_infos = NULL;
  int requested_protocols[2] = {AF_INET6, 0};

  int err = 0;
  int ret = 0;
  // Check for protocols in a do-while loop until we provide a buffer large
  // enough. (WSCEnumProtocols sets protbuff_size to its desired value).
  // It is extremely unlikely that this will loop more than once.
  do {
    protocols.reset(new char[protbuff_size]);
    protocol_infos = reinterpret_cast<LPWSAPROTOCOL_INFOW>(protocols.get());
    ret = WSCEnumProtocols(requested_protocols, protocol_infos,
                           &protbuff_size, &err);
  } while (ret == SOCKET_ERROR && err == WSAENOBUFS);

  if (ret == SOCKET_ERROR) {
    return false;
  }

  // Even if ret is positive, check specifically for IPv6.
  // Non-IPv6 enabled WinXP will still return a RAW protocol.
  for (int i = 0; i < ret; ++i) {
    if (protocol_infos[i].iAddressFamily == AF_INET6) {
      return true;
    }
  }
  return false;
#endif
}
}  // namespace vzes
