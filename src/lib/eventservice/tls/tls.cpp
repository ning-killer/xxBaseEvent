//

#include "eventservice/tls/tls.h"
#include "eventservice/event/thread.h"

#include <map>


namespace vzes {

#ifdef WIN32
Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  *key = TlsAlloc();
}

void Tls::DeleteKey(TLS_KEY key) {
  TlsFree(key);
}

void Tls::SetKey(TLS_KEY key, void *value) {
  TlsSetValue(key, value);
}

void *Tls::GetKey(TLS_KEY key) {
  return TlsGetValue(key);
}
#elif LITEOS

typedef std::map<pthread_t, void *> LiteOSKeys;

Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  *key = (void *)(new LiteOSKeys());
}

void Tls::DeleteKey(TLS_KEY key) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  delete liteos_keys;
}

void Tls::SetKey(TLS_KEY key, void *value) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  pthread_t id = pthread_self();
  (*liteos_keys)[id] = value;
}

void *Tls::GetKey(TLS_KEY key) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  pthread_t id = pthread_self();
  return (*liteos_keys)[id];
}
#elif POSIX
Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  pthread_key_create(key, NULL);
}

void Tls::DeleteKey(TLS_KEY key) {
  pthread_key_delete(key);
}

void Tls::SetKey(TLS_KEY key, void *value) {
  pthread_setspecific(key, value);
}

void *Tls::GetKey(TLS_KEY key) {
  return pthread_getspecific(key);
}

//typedef std::map<pthread_t, void *> LiteOSKeys;
//
//Tls::Tls() {
//}
//
//Tls::~Tls() {
//}
//
//void Tls::CreateKey(TLS_KEY *key) {
//  *key = (void *)(new LiteOSKeys());
//}
//
//void Tls::DeleteKey(TLS_KEY key) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  delete liteos_keys;
//}
//
//void Tls::SetKey(TLS_KEY key, void *value) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  pthread_t id = pthread_self();
//  (*liteos_keys)[id] = value;
//}
//
//void *Tls::GetKey(TLS_KEY key) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  pthread_t id = pthread_self();
//  return (*liteos_keys)[id];
//}
#endif
}  // namespace vzes
