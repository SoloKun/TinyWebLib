#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}
 private:
  noncopyable(const noncopyable&);//拷贝构造函数
  const noncopyable& operator=(const noncopyable&);//赋值构造函数
};

#endif