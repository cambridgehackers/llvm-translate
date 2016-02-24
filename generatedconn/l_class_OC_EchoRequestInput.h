#ifndef __l_class_OC_EchoRequestInput_H__
#define __l_class_OC_EchoRequestInput_H__
#include "l_class_OC_EchoRequest.h"
#include "l_struct_OC_EchoRequest_data.h"
class l_class_OC_EchoRequestInput {
  l_class_OC_EchoRequest *request;
public:
  void run();
  void enq(l_struct_OC_EchoRequest_data enq_v);
  bool enq__RDY(void);
  void setrequest(l_class_OC_EchoRequest *v) { request = v; }
};
#endif  // __l_class_OC_EchoRequestInput_H__
