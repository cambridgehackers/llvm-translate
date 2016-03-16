#ifndef __l_class_OC_EchoIndicationInput_H__
#define __l_class_OC_EchoIndicationInput_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_PipeIn_OC_0.h"
#include "l_struct_OC_EchoIndication_data.h"
class l_class_OC_EchoIndicationInput;
extern void l_class_OC_EchoIndicationInput__enq(void *thisarg, l_struct_OC_EchoIndication_data enq_v);
extern bool l_class_OC_EchoIndicationInput__enq__RDY(void *thisarg);
class l_class_OC_EchoIndicationInput {
public:
  l_class_OC_PipeIn_OC_0 pipe;
  l_class_OC_EchoIndication *indication;
public:
  void run();
  void commit();
  l_class_OC_EchoIndicationInput():
      pipe(this, l_class_OC_EchoIndicationInput__enq__RDY, l_class_OC_EchoIndicationInput__enq) {
  }
  void setindication(l_class_OC_EchoIndication *v) { indication = v; }
};
#endif  // __l_class_OC_EchoIndicationInput_H__
