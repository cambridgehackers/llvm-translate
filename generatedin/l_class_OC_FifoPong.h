#ifndef __l_class_OC_FifoPong_H__
#define __l_class_OC_FifoPong_H__
#include "l_class_OC_Fifo1_OC_3.h"
#include "l_class_OC_Fifo_OC_0.h"
#include "l_struct_OC_ValuePair.h"
class l_class_OC_FifoPong;
extern void l_class_OC_FifoPong__deq(void *thisarg);
extern bool l_class_OC_FifoPong__deq__RDY(void *thisarg);
extern void l_class_OC_FifoPong__enq(void *thisarg, l_struct_OC_ValuePair enq_v);
extern bool l_class_OC_FifoPong__enq__RDY(void *thisarg);
extern l_struct_OC_ValuePair l_class_OC_FifoPong__first(void *thisarg);
extern bool l_class_OC_FifoPong__first__RDY(void *thisarg);
class l_class_OC_FifoPong {
public:
  l_class_OC_PipeIn_OC_1 in;
  l_class_OC_PipeOut_OC_2 out;
  l_class_OC_Fifo1_OC_3 element1;
  l_class_OC_Fifo1_OC_3 element2;
  bool pong, pong_shadow; bool pong_valid;
public:
  void run();
  void commit();
  l_class_OC_FifoPong():
      in(this, l_class_OC_FifoPong__enq__RDY, l_class_OC_FifoPong__enq),
      out(this, l_class_OC_FifoPong__deq__RDY, l_class_OC_FifoPong__deq, l_class_OC_FifoPong__first__RDY, l_class_OC_FifoPong__first) {
  }
};
#endif  // __l_class_OC_FifoPong_H__
