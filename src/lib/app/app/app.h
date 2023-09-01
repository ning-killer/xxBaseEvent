#ifndef APP_APP_APP_H_
#define APP_APP_APP_H_

#include "app/app/appinterface.h"

namespace app {

class App {
 public:
  typedef boost::shared_ptr<App> Ptr;
 public:
  // Create the application process framework.
  // multi_proc_log: whether the Log module supports cross-process logging.
  static App::Ptr CreateApp(bool multi_proc_log = false);
  virtual bool RegisterApp(AppInterface::Ptr app) = 0;
  // Initialize and run all modules. 
  // internal_blocking: Whether the main thread is blocked inside 
  // the VzBaseEvent library, if not, it needs to be blocked inside 
  // the main() function, otherwise the process will exits.
  virtual void AppRun(bool internal_blocking = true) = 0;
  virtual void ExitApp() = 0;
};

}

#endif  // APP_APP_APP_H_