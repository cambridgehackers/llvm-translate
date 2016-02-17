#ifndef __l_class_OC_MMURequestInput_H__
#define __l_class_OC_MMURequestInput_H__
#include "l_class_OC_MMURequest.h"
class l_class_OC_MMURequestInput {
  l_class_OC_MMURequest *request;
public:
  void run();
  void setrequest(l_class_OC_MMURequest *v) { request = v; }
};
#endif  // __l_class_OC_MMURequestInput_H__
