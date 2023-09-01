//


#ifndef EVENTSERVICE_EVENT_MESSAGEHANDLER_H_
#define EVENTSERVICE_EVENT_MESSAGEHANDLER_H_

#include "eventservice/base/constructormagic.h"

namespace vzes {

struct Message;

// Messages get dispatched to a MessageHandler

class MessageHandler {
 public:
  virtual void OnMessage(Message* msg) = 0;

 protected:
  MessageHandler() {}
  virtual ~MessageHandler();

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};

} // namespace vzes

#endif // EVENTSERVICE_EVENT_MESSAGEHANDLER_H_
