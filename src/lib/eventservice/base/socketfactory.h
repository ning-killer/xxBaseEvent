//

#ifndef EVENTSERVICES_BASE_SOCKETFACTORY_H__
#define EVENTSERVICES_BASE_SOCKETFACTORY_H__

#include "eventservice/base/socket.h"

namespace vzes {

class SocketFactory {
 public:
  virtual ~SocketFactory() {}

  // Returns a new socket for blocking communication.  The type can be
  // SOCK_DGRAM and SOCK_STREAM.
  // TODO: C++ inheritance rules mean that all users must have both
  // CreateSocket(int) and CreateSocket(int,int). Will remove CreateSocket(int)
  // (and CreateAsyncSocket(int) when all callers are changed.
  virtual Socket::Ptr CreateSocket(int type) = 0;
  virtual Socket::Ptr CreateSocket(int family, int type) = 0;
  // Returns a new socket for nonblocking communication.  The type can be
  // SOCK_DGRAM and SOCK_STREAM.
  //virtual AsyncSocket* CreateAsyncSocket(int type) = 0;
  //virtual AsyncSocket* CreateAsyncSocket(int family, int type) = 0;
};

} // namespace vzes

#endif // EVENTSERVICES_BASE_SOCKETFACTORY_H__
