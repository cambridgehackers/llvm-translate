#ifndef __l_class_OC_EchoRequestInput_H__
#define __l_class_OC_EchoRequestInput_H__
#include "l_class_OC_EchoRequest.h"
#include "l_struct_OC_EchoRequest_data.h"
class l_class_OC_EchoRequestInput;
extern void l_class_OC_EchoRequestInput__enq(l_class_OC_EchoRequestInput *thisp, l_struct_OC_EchoRequest_data enq_v);
extern bool l_class_OC_EchoRequestInput__enq__RDY(l_class_OC_EchoRequestInput *thisp);
class l_class_OC_EchoRequestInput {
public:
  l_class_OC_EchoRequest *request;
public:
  void run();
  void enq(l_struct_OC_EchoRequest_data enq_v) { l_class_OC_EchoRequestInput__enq(this, enq_v); }
  bool enq__RDY(void) { return l_class_OC_EchoRequestInput__enq__RDY(this); }
  void setrequest(l_class_OC_EchoRequest *v) { request = v; }
};
#endif  // __l_class_OC_EchoRequestInput_H__
