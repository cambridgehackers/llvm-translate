#ifndef __l_class_OC_IVectorTest_H__
#define __l_class_OC_IVectorTest_H__
#include "l_class_OC_IVector.h"
class l_class_OC_IVectorTest {
  l_class_OC_IVector *ivector;
public:
  void run();
  void setivector(l_class_OC_IVector *v) { ivector = v; }
};
#endif  // __l_class_OC_IVectorTest_H__
