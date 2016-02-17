#ifndef __l_class_OC_MemServerIndicationOutput_H__
#define __l_class_OC_MemServerIndicationOutput_H__
#include "l_class_OC_MemServerIndication.h"
#include "l_class_OC_Pipes.h"
class l_class_OC_MemServerIndicationOutput {
  l_class_OC_MemServerIndication ifc;
  l_class_OC_Pipes pipes;
public:
  void run();
};
#endif  // __l_class_OC_MemServerIndicationOutput_H__
