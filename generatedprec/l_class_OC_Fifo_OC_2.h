#ifndef __l_class_OC_Fifo_OC_2_H__
#define __l_class_OC_Fifo_OC_2_H__
#include "l_struct_OC_ValueType.h"
class l_class_OC_Fifo_OC_2;
extern void l_class_OC_Fifo_OC_2__in_enq(void *thisarg, l_struct_OC_ValueType in_enq_v);
extern bool l_class_OC_Fifo_OC_2__in_enq__RDY(void *thisarg);
extern void l_class_OC_Fifo_OC_2__out_deq(void *thisarg);
extern bool l_class_OC_Fifo_OC_2__out_deq__RDY(void *thisarg);
extern l_struct_OC_ValueType l_class_OC_Fifo_OC_2__out_first(void *thisarg);
extern bool l_class_OC_Fifo_OC_2__out_first__RDY(void *thisarg);
class l_class_OC_Fifo_OC_2 {
public:
public:
  void run();
  void commit();
  void in_enq(l_struct_OC_ValueType in_enq_v) { l_class_OC_Fifo_OC_2__in_enq(this, in_enq_v); }
  bool in_enq__RDY(void) { return l_class_OC_Fifo_OC_2__in_enq__RDY(this); }
  void out_deq(void) { l_class_OC_Fifo_OC_2__out_deq(this); }
  bool out_deq__RDY(void) { return l_class_OC_Fifo_OC_2__out_deq__RDY(this); }
  l_struct_OC_ValueType out_first(void) { return l_class_OC_Fifo_OC_2__out_first(this); }
  bool out_first__RDY(void) { return l_class_OC_Fifo_OC_2__out_first__RDY(this); }
};
#endif  // __l_class_OC_Fifo_OC_2_H__
