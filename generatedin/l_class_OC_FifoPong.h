#ifndef __l_class_OC_FifoPong_H__
#define __l_class_OC_FifoPong_H__
#include "l_class_OC_Fifo1_OC_3.h"
#include "l_struct_OC_ValuePair.h"
class l_class_OC_FifoPong;
extern void l_class_OC_FifoPong__in_enq(l_class_OC_FifoPong *thisp, l_struct_OC_ValuePair in_enq_v);
extern bool l_class_OC_FifoPong__in_enq__RDY(l_class_OC_FifoPong *thisp);
extern void l_class_OC_FifoPong__out_deq(l_class_OC_FifoPong *thisp);
extern bool l_class_OC_FifoPong__out_deq__RDY(l_class_OC_FifoPong *thisp);
extern l_struct_OC_ValuePair l_class_OC_FifoPong__out_first(l_class_OC_FifoPong *thisp);
extern bool l_class_OC_FifoPong__out_first__RDY(l_class_OC_FifoPong *thisp);
class l_class_OC_FifoPong {
public:
  l_class_OC_Fifo1_OC_3 element1;
  l_class_OC_Fifo1_OC_3 element2;
  bool pong, pong_shadow; bool pong_valid;
public:
  void run();
  void commit();
  void in_enq(l_struct_OC_ValuePair in_enq_v) { l_class_OC_FifoPong__in_enq(this, in_enq_v); }
  bool in_enq__RDY(void) { return l_class_OC_FifoPong__in_enq__RDY(this); }
  void out_deq(void) { l_class_OC_FifoPong__out_deq(this); }
  bool out_deq__RDY(void) { return l_class_OC_FifoPong__out_deq__RDY(this); }
  l_struct_OC_ValuePair out_first(void) { return l_class_OC_FifoPong__out_first(this); }
  bool out_first__RDY(void) { return l_class_OC_FifoPong__out_first__RDY(this); }
};
#endif  // __l_class_OC_FifoPong_H__
