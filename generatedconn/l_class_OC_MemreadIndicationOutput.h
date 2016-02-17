#ifndef __l_class_OC_MemreadIndicationOutput_H__
#define __l_class_OC_MemreadIndicationOutput_H__
#include "l_class_OC_MemreadIndication.h"
#include "l_class_OC_Pipes.h"
class l_class_OC_MemreadIndicationOutput {
  l_class_OC_MemreadIndication ifc;
  l_class_OC_Pipes pipes;
public:
  void run();
};
#endif  // __l_class_OC_MemreadIndicationOutput_H__
