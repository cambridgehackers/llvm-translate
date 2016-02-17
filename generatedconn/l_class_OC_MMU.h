#ifndef __l_class_OC_MMU_H__
#define __l_class_OC_MMU_H__
#include "l_class_OC_MMURequest.h"
class l_class_OC_MMU {
  l_class_OC_MMURequest request;
public:
  void run();
};
#endif  // __l_class_OC_MMU_H__
