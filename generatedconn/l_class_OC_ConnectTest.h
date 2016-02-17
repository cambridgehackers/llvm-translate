#ifndef __l_class_OC_ConnectTest_H__
#define __l_class_OC_ConnectTest_H__
#include "l_class_OC_Connect.h"
class l_class_OC_ConnectTest {
  l_class_OC_Connect *connect;
public:
  void run();
  void setconnect(l_class_OC_Connect *v) { connect = v; }
};
#endif  // __l_class_OC_ConnectTest_H__
