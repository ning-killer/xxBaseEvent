//


#ifndef SRC_BASE_NONCOPYABLE_H_
#define SRC_BASE_NONCOPYABLE_H_


namespace vzes {

class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  // emphasize the following members are private
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};
}  // namespace vzes

#endif  // SRC_BASE_NONCOPYABLE_H_
