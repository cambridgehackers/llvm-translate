#ifndef __l_class_OC_Fifo_OC_1_H__
#define __l_class_OC_Fifo_OC_1_H__
#include "l_unnamed_2.h"
class l_class_OC_Fifo_OC_1 {
public:
  void run();
  void in_enq(bool in_enq_v_2e_coerce0, bool in_enq_v_2e_coerce1);
  bool in_enq__RDY(void);
  void out_deq(void);
  bool out_deq__RDY(void);
  l_unnamed_2 out_first(void);
  bool out_first__RDY(void);
};
#endif  // __l_class_OC_Fifo_OC_1_H__
