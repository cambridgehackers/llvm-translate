#ifndef __l_class_OC_EchoTest_H__
#define __l_class_OC_EchoTest_H__
#include "l_class_OC_Echo.h"
class l_class_OC_EchoTest;
class l_class_OC_EchoTest {
public:
  l_class_OC_Echo echo;
  unsigned int x, x_shadow; bool x_valid;
public:
  void run();
  void commit();
};
#endif  // __l_class_OC_EchoTest_H__
