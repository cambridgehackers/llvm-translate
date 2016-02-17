#ifndef __l_class_OC_MMUIndicationOutput_H__
#define __l_class_OC_MMUIndicationOutput_H__
#include "l_class_OC_MMUIndication.h"
#include "l_class_OC_Pipes.h"
class l_class_OC_MMUIndicationOutput {
  l_class_OC_MMUIndication ifc;
  l_class_OC_Pipes pipes;
public:
  void run();
};
#endif  // __l_class_OC_MMUIndicationOutput_H__
