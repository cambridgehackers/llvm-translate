#ifndef __l_class_OC_FifoPong_H__
#define __l_class_OC_FifoPong_H__
#include "l_class_OC_Fifo1_OC_4.h"
#include "l_struct_OC_ValuePair.h"
class l_class_OC_FifoPong {
  l_class_OC_Fifo1_OC_4 element1;
public:
  void run();
  void in_enq(l_struct_OC_ValuePair in_enq_v);
  bool in_enq__RDY(void);
  void out_deq(void);
  bool out_deq__RDY(void);
  l_struct_OC_ValuePair out_first(void);
  bool out_first__RDY(void);
};
#endif  // __l_class_OC_FifoPong_H__
