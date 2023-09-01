//

#ifndef SRC_BASE_HELPMETHODS_H_
#define SRC_BASE_HELPMETHODS_H_

#include <string>
#include "astl/include/string.hpp"
#include "eventservice/base/basictypes.h"

namespace vzes {

enum HttpMethod {
  HTTP_GET,
  HTTP_POST
};

class HelpMethods {
 public:
  static const std::string GetCurrentUTCTime();
  static const std::string GetSignatureNonce();
  static void Uint64ToString(uint64 n, std::string &str); // NOLINT
  static const std::string GetRandNumString(int size);
  static uint32 GetUnixTimeStamp();
  static const std::string URLEncode(const std::string &str);

  static const std::string HttpMethodToStr(HttpMethod hm);

  static void HmacSha1(const uint8 *key, std::size_t key_size,
                       const uint8 *data, std::size_t data_size, uint8 *result);

  static void HmacSha1ToBase64(const std::string &key, // NOLINT
                               const std::string &data, // NOLINT
                               std::string &result); // NOLINT
  // int转str
  static std::string IntToStr(int n);
  // 获取随机字符串
  static std::string GetRandomString(std::size_t size);
};

}  // namespace vzes
#endif  // SRC_BASE_HELPMETHODS_H_
