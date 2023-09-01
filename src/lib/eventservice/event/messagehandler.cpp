//

#include "eventservice/event/messagehandler.h"
#include "eventservice/event/messagequeue.h"

namespace vzes {

MessageHandler::~MessageHandler() {
  MessageQueueManager::Clear(this);
}

} // namespace vzes
