#ifndef __l_class_OC_Fifo2_H__
#define __l_class_OC_Fifo2_H__
#include "l_class_OC_Fifo_OC_1.h"
#include "l_struct_OC_ValuePair.h"
class l_class_OC_Fifo2;
extern void l_class_OC_Fifo2__deq(void *thisarg);
extern bool l_class_OC_Fifo2__deq__RDY(void *thisarg);
extern void l_class_OC_Fifo2__enq(void *thisarg, l_struct_OC_ValuePair enq_v);
extern bool l_class_OC_Fifo2__enq__RDY(void *thisarg);
extern l_struct_OC_ValuePair l_class_OC_Fifo2__first(void *thisarg);
extern bool l_class_OC_Fifo2__first__RDY(void *thisarg);
class l_class_OC_Fifo2 {
public:
  l_class_OC_PipeIn_OC_2 in;
  l_class_OC_PipeOut_OC_3 out;
  l_struct_OC_ValuePair element0;
  l_struct_OC_ValuePair element1;
  l_struct_OC_ValuePair element2;
  unsigned int rindex, rindex_shadow; bool rindex_valid;
  unsigned int windex, windex_shadow; bool windex_valid;
public:
  void run();
  void commit();
  l_class_OC_Fifo2():
      in(this, l_class_OC_Fifo2__enq__RDY, l_class_OC_Fifo2__enq),
      out(this, l_class_OC_Fifo2__deq__RDY, l_class_OC_Fifo2__deq, l_class_OC_Fifo2__first__RDY, l_class_OC_Fifo2__first) {
  }
};
#endif  // __l_class_OC_Fifo2_H__
