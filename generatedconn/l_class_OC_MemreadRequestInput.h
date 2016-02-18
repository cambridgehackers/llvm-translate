#ifndef __l_class_OC_MemreadRequestInput_H__
#define __l_class_OC_MemreadRequestInput_H__
#include "l_class_OC_MemreadRequest.h"
#include "l_struct_OC_MemreadRequest_data.h"
class l_class_OC_MemreadRequestInput {
  l_class_OC_MemreadRequest *request;
public:
  void run();
  void pipe_enq(l_struct_OC_MemreadRequest_data pipe_enq_v);
  bool pipe_enq__RDY(void);
  void setrequest(l_class_OC_MemreadRequest *v) { request = v; }
};
#endif  // __l_class_OC_MemreadRequestInput_H__
