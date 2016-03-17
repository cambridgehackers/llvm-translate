#ifndef __l_class_OC_Fifo_H__
#define __l_class_OC_Fifo_H__
#include "l_class_OC_PipeIn.h"
#include "l_class_OC_PipeOut.h"
class l_class_OC_Fifo;
extern void l_class_OC_Fifo__deq(void *thisarg);
extern bool l_class_OC_Fifo__deq__RDY(void *thisarg);
extern void l_class_OC_Fifo__enq(void *thisarg, unsigned int enq_v);
extern bool l_class_OC_Fifo__enq__RDY(void *thisarg);
extern unsigned int l_class_OC_Fifo__first(void *thisarg);
extern bool l_class_OC_Fifo__first__RDY(void *thisarg);
class l_class_OC_Fifo {
public:
  l_class_OC_PipeIn in;
  l_class_OC_PipeOut out;
public:
  void run();
  void commit();
  l_class_OC_Fifo():
      in(this, l_class_OC_Fifo__enq__RDY, l_class_OC_Fifo__enq),
      out(this, l_class_OC_Fifo__deq__RDY, l_class_OC_Fifo__deq, l_class_OC_Fifo__first__RDY, l_class_OC_Fifo__first) {
  }
};
#endif  // __l_class_OC_Fifo_H__
