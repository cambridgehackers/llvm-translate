#ifndef __l_class_OC_Fifo1_OC_0_H__
#define __l_class_OC_Fifo1_OC_0_H__
#include "l_class_OC_Fifo_OC_1.h"
#include "l_struct_OC_ValueType.h"
class l_class_OC_Fifo1_OC_0;
extern void l_class_OC_Fifo1_OC_0__deq(void *thisarg);
extern bool l_class_OC_Fifo1_OC_0__deq__RDY(void *thisarg);
extern void l_class_OC_Fifo1_OC_0__enq(void *thisarg, l_struct_OC_ValueType enq_v);
extern bool l_class_OC_Fifo1_OC_0__enq__RDY(void *thisarg);
extern l_struct_OC_ValueType l_class_OC_Fifo1_OC_0__first(void *thisarg);
extern bool l_class_OC_Fifo1_OC_0__first__RDY(void *thisarg);
class l_class_OC_Fifo1_OC_0 {
public:
  l_class_OC_PipeIn_OC_2 in;
  l_class_OC_PipeOut_OC_3 out;
  l_struct_OC_ValueType element;
  bool full, full_shadow; bool full_valid;
public:
  void run();
  void commit();
  l_class_OC_Fifo1_OC_0():
      in(this, l_class_OC_Fifo1_OC_0__enq__RDY, l_class_OC_Fifo1_OC_0__enq),
      out(this, l_class_OC_Fifo1_OC_0__deq__RDY, l_class_OC_Fifo1_OC_0__deq, l_class_OC_Fifo1_OC_0__first__RDY, l_class_OC_Fifo1_OC_0__first) {
  }
};
#endif  // __l_class_OC_Fifo1_OC_0_H__
