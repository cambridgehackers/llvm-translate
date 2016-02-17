#ifndef __l_class_OC_MemreadRequestInput_H__
#define __l_class_OC_MemreadRequestInput_H__
#include "l_class_OC_MemreadRequest.h"
class l_class_OC_MemreadRequestInput {
  l_class_OC_MemreadRequest *request;
public:
  void run();
  void setrequest(l_class_OC_MemreadRequest *v) { request = v; }
};
#endif  // __l_class_OC_MemreadRequestInput_H__
