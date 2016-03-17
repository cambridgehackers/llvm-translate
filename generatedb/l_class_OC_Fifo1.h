#ifndef __l_class_OC_Fifo1_H__
#define __l_class_OC_Fifo1_H__
#include "l_class_OC_Fifo.h"
class l_class_OC_Fifo1;
extern void l_class_OC_Fifo1__deq(void *thisarg);
extern bool l_class_OC_Fifo1__deq__RDY(void *thisarg);
extern void l_class_OC_Fifo1__enq(void *thisarg, unsigned int enq_v);
extern bool l_class_OC_Fifo1__enq__RDY(void *thisarg);
extern unsigned int l_class_OC_Fifo1__first(void *thisarg);
extern bool l_class_OC_Fifo1__first__RDY(void *thisarg);
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
      in(this, l_class_OC_Fifo1__enq__RDY, l_class_OC_Fifo1__enq),
      out(this, l_class_OC_Fifo1__deq__RDY, l_class_OC_Fifo1__deq, l_class_OC_Fifo1__first__RDY, l_class_OC_Fifo1__first) {
  }
};
#endif  // __l_class_OC_Fifo1_H__
