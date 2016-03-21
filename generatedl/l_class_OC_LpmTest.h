#ifndef __l_class_OC_LpmTest_H__
#define __l_class_OC_LpmTest_H__
#include "l_class_OC_Lpm.h"
class l_class_OC_LpmTest;
class l_class_OC_LpmTest {
public:
  l_class_OC_Lpm *lpm;
public:
  void run();
  void commit();
  void setlpm(l_class_OC_Lpm *v) { lpm = v; }
};
#endif  // __l_class_OC_LpmTest_H__
