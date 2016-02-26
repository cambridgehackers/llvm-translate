#ifndef __l_class_OC_EchoIndicationInput_H__
#define __l_class_OC_EchoIndicationInput_H__
#include "l_class_OC_foo.h"
#include "l_struct_OC_EchoIndication_data.h"
class l_class_OC_EchoIndicationInput;
extern void l_class_OC_EchoIndicationInput__enq(l_class_OC_EchoIndicationInput *thisp, l_struct_OC_EchoIndication_data enq_v);
extern bool l_class_OC_EchoIndicationInput__enq__RDY(l_class_OC_EchoIndicationInput *thisp);
class l_class_OC_EchoIndicationInput {
public:
  l_class_OC_foo *request0;
public:
  void run();
  void enq(l_struct_OC_EchoIndication_data enq_v) { l_class_OC_EchoIndicationInput__enq(this, enq_v); }
  bool enq__RDY(void) { return l_class_OC_EchoIndicationInput__enq__RDY(this); }
  void setrequest(l_class_OC_foo *v) { request = v; }
};
#endif  // __l_class_OC_EchoIndicationInput_H__
