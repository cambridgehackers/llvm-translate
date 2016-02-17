#ifndef __l_class_OC_MemreadRequestInput_H__
#define __l_class_OC_MemreadRequestInput_H__
#include "l_class_OC_MemreadRequest.h"
class l_class_OC_MemreadRequestInput {
  l_class_OC_MemreadRequest *request;
public:
  void run();
  void pipe_enq(unsigned int pipe_enq_v_2e_coerce);
  bool pipe_enq__RDY(void);
  void setrequest(l_class_OC_MemreadRequest *v) { request = v; }
};
#endif  // __l_class_OC_MemreadRequestInput_H__
