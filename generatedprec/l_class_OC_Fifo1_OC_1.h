#ifndef __l_class_OC_Fifo1_OC_1_H__
#define __l_class_OC_Fifo1_OC_1_H__
#include "l_class_OC_Fifo_OC_2.h"
#include "l_class_OC_PipeIn_OC_3.h"
#include "l_class_OC_PipeOut_OC_4.h"
#include "l_struct_OC_ValueType.h"
class l_class_OC_Fifo1_OC_1;
extern void l_class_OC_Fifo1_OC_1__deq(void *thisarg);
extern bool l_class_OC_Fifo1_OC_1__deq__RDY(void *thisarg);
extern void l_class_OC_Fifo1_OC_1__enq(void *thisarg, l_struct_OC_ValueType enq_v);
extern bool l_class_OC_Fifo1_OC_1__enq__RDY(void *thisarg);
extern l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__first(void *thisarg);
extern bool l_class_OC_Fifo1_OC_1__first__RDY(void *thisarg);
class l_class_OC_Fifo1_OC_1 {
public:
  l_class_OC_PipeIn_OC_3 in;
  l_class_OC_PipeOut_OC_4 out;
  l_struct_OC_ValueType element;
  bool full, full_shadow; bool full_valid;
public:
  void run();
  void commit();
  l_class_OC_Fifo1_OC_1():
      in(this, l_class_OC_Fifo1_OC_1__enq__RDY, l_class_OC_Fifo1_OC_1__enq),
      out(this, l_class_OC_Fifo1_OC_1__deq__RDY, l_class_OC_Fifo1_OC_1__deq, l_class_OC_Fifo1_OC_1__first__RDY, l_class_OC_Fifo1_OC_1__first) {
  }
};
#endif  // __l_class_OC_Fifo1_OC_1_H__
