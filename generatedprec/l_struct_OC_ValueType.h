#ifndef __l_struct_OC_ValueType_H__
#define __l_struct_OC_ValueType_H__
#include "l_struct_OC_ValueType.h"
typedef struct {
public:
  BITS6 a;
  BITS4 b;
public:
  void run();
  void ValueType(l_struct_OC_ValueType tmp__1) { l_struct_OC_ValueType__ValueType(this, tmp__1); }
  l_struct_OC_ValueType *operator=(l_struct_OC_ValueType tmp__2) { return l_struct_OC_ValueType__operator=(this, tmp__2); }
}l_struct_OC_ValueType;
#endif  // __l_struct_OC_ValueType_H__
