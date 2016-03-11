#ifndef __l_class_OC_Fifo1_OC_1_H__
#define __l_class_OC_Fifo1_OC_1_H__
#include "l_struct_OC_ValueType.h"
class l_class_OC_Fifo1_OC_1;
extern void l_class_OC_Fifo1_OC_1__in_enq(l_class_OC_Fifo1_OC_1 *thisp, l_struct_OC_ValueType in_enq_v);
extern bool l_class_OC_Fifo1_OC_1__in_enq__RDY(l_class_OC_Fifo1_OC_1 *thisp);
extern void l_class_OC_Fifo1_OC_1__out_deq(l_class_OC_Fifo1_OC_1 *thisp);
extern bool l_class_OC_Fifo1_OC_1__out_deq__RDY(l_class_OC_Fifo1_OC_1 *thisp);
extern l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__out_first(l_class_OC_Fifo1_OC_1 *thisp);
extern bool l_class_OC_Fifo1_OC_1__out_first__RDY(l_class_OC_Fifo1_OC_1 *thisp);
class l_class_OC_Fifo1_OC_1 {
public:
  l_struct_OC_ValueType element;
  bool full, full_shadow; bool full_valid;
public:
  void run();
  void commit();
  void in_enq(l_struct_OC_ValueType in_enq_v) { l_class_OC_Fifo1_OC_1__in_enq(this, in_enq_v); }
  bool in_enq__RDY(void) { return l_class_OC_Fifo1_OC_1__in_enq__RDY(this); }
  void out_deq(void) { l_class_OC_Fifo1_OC_1__out_deq(this); }
  bool out_deq__RDY(void) { return l_class_OC_Fifo1_OC_1__out_deq__RDY(this); }
  l_struct_OC_ValueType out_first(void) { return l_class_OC_Fifo1_OC_1__out_first(this); }
  bool out_first__RDY(void) { return l_class_OC_Fifo1_OC_1__out_first__RDY(this); }
};
#endif  // __l_class_OC_Fifo1_OC_1_H__
