#ifndef __l_class_OC_PipeIn_OC_3_H__
#define __l_class_OC_PipeIn_OC_3_H__
#include "l_struct_OC_ValueType.h"
class l_class_OC_PipeIn_OC_3 {
public:
  void *p;
  bool  (*enq__RDYp) (void *);
  void  (*enqp) (void *, l_struct_OC_ValueType );
public:
  void enq(l_struct_OC_ValueType v) { enqp(p, v); }
  bool enq__RDY(void) { return enq__RDYp(p); }
  l_class_OC_PipeIn_OC_3(decltype(p) ap, decltype(enq__RDYp) aenq__RDYp, decltype(enqp) aenqp) {
    p = ap;
    enq__RDYp = aenq__RDYp;
    enqp = aenqp;
  }
};
#endif  // __l_class_OC_PipeIn_OC_3_H__
