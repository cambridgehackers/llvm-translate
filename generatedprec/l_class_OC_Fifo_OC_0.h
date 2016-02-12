#ifndef __l_class_OC_Fifo_OC_0_H__
#define __l_class_OC_Fifo_OC_0_H__
#include "l_struct_OC_ValuePair.h"
class l_class_OC_Fifo_OC_0 {
public:
  void run();
  void in_enq(l_struct_OC_ValuePair in_enq_v);
  bool in_enq__RDY(void);
  void out_deq(void);
  bool out_deq__RDY(void);
  l_struct_OC_ValuePair out_first(void);
  bool out_first__RDY(void);
};
#endif  // __l_class_OC_Fifo_OC_0_H__
