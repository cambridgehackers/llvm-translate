#ifndef __l_class_OC_PipeOut_OC_4_H__
#define __l_class_OC_PipeOut_OC_4_H__
#include "l_struct_OC_ValueType.h"
class l_class_OC_PipeOut_OC_4 {
public:
  void *p;
  bool  (*deq__RDYp) (void *);
  void  (*deqp) (void *);
  bool  (*first__RDYp) (void *);
  l_struct_OC_ValueType  (*firstp) (void *);
public:
  void deq(void) { deqp(p); }
  bool deq__RDY(void) { return deq__RDYp(p); }
  l_struct_OC_ValueType first(void) { return firstp(p); }
  bool first__RDY(void) { return first__RDYp(p); }
  l_class_OC_PipeOut_OC_4(decltype(p) ap, decltype(deq__RDYp) adeq__RDYp, decltype(deqp) adeqp, decltype(first__RDYp) afirst__RDYp, decltype(firstp) afirstp) {
    p = ap;
    deq__RDYp = adeq__RDYp;
    deqp = adeqp;
    first__RDYp = afirst__RDYp;
    firstp = afirstp;
  }
};
#endif  // __l_class_OC_PipeOut_OC_4_H__
