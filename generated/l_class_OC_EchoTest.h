#ifndef __l_class_OC_EchoTest_H__
#define __l_class_OC_EchoTest_H__
#include "l_class_OC_Echo.h"
class l_class_OC_EchoTest {
public:
  l_class_OC_Echo *echo;
  unsigned int x;
public:
  void run();
  void setecho(l_class_OC_Echo *v) { echo = v; }
};
#endif  // __l_class_OC_EchoTest_H__
