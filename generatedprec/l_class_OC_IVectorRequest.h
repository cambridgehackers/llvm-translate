#ifndef __l_class_OC_IVectorRequest_H__
#define __l_class_OC_IVectorRequest_H__
#include "l_class_OC_FixedPoint.h"
#include "l_class_OC_FixedPoint_OC_0.h"
class l_class_OC_IVectorRequest {
public:
  void run();
  void say(BITS say_meth, BITS say_v);
  bool say__RDY(void);
};
#endif  // __l_class_OC_IVectorRequest_H__
