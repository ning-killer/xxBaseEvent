/*
* vzsdk
* Copyright 2013 - 2018, Vzenith Inc.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*  3. The name of the author may not be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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