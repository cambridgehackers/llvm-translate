#ifndef __l_struct_OC_ValuePair_H__
#define __l_struct_OC_ValuePair_H__
#include "l_class_OC_FixedPoint.h"
typedef struct {
  l_class_OC_FixedPoint a;
  l_class_OC_FixedPoint b;
  unsigned int c[20];
public:
  void run();
}l_struct_OC_ValuePair;
#endif  // __l_struct_OC_ValuePair_H__
