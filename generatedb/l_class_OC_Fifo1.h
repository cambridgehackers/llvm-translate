#ifndef __l_class_OC_Fifo1_H__
#define __l_class_OC_Fifo1_H__
#include "l_class_OC_Fifo.h"
class l_class_OC_Fifo1;
extern void l_class_OC_Fifo1__in_enq(void *thisarg, unsigned int in_enq_v);
extern bool l_class_OC_Fifo1__in_enq__RDY(void *thisarg);
extern void l_class_OC_Fifo1__out_deq(void *thisarg);
extern bool l_class_OC_Fifo1__out_deq__RDY(void *thisarg);
extern unsigned int l_class_OC_Fifo1__out_first(void *thisarg);
extern bool l_class_OC_Fifo1__out_first__RDY(void *thisarg);
class l_class_OC_Fifo1 {
public:
  l_class_OC_PipeIn in;
  l_class_OC_PipeOut out;
  unsigned int element, element_shadow; bool element_valid;
  bool full, full_shadow; bool full_valid;
public:
  void run();
  void commit();
  l_class_OC_Fifo1():
      in(this, l_class_OC_Fifo1__in_enq__RDY, l_class_OC_Fifo1__in_enq),
      out(this, l_class_OC_Fifo1__out_deq__RDY, l_class_OC_Fifo1__out_deq, l_class_OC_Fifo1__out_first__RDY, l_class_OC_Fifo1__out_first) {
  }
};
#endif  // __l_class_OC_Fifo1_H__
