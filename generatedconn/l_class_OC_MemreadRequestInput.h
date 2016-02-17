#ifndef __l_class_OC_MemreadRequestInput_H__
#define __l_class_OC_MemreadRequestInput_H__
#include "l_class_OC_MemreadRequest.h"
#include "l_class_OC_Pipes.h"
class l_class_OC_MemreadRequestInput {
  l_class_OC_MemreadRequest ifc;
  l_class_OC_Pipes pipes;
public:
  void run();
};
#endif  // __l_class_OC_MemreadRequestInput_H__
