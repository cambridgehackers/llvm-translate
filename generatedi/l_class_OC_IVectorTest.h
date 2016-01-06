#ifndef __l_class_OC_IVectorTest_H__
#define __l_class_OC_IVectorTest_H__
#include "l_class_OC_IVector.h"
class l_class_OC_IVectorTest {
  class l_class_OC_IVector *ivector;
  unsigned int x;
public:
  void run();
  void setivector(class l_class_OC_IVector *v) { ivector = v; }
};
#endif  // __l_class_OC_IVectorTest_H__
