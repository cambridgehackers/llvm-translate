#ifndef __l_class_OC_FifoPong_H__
#define __l_class_OC_FifoPong_H__
#include "l_class_OC_Fifo1_OC_3.h"
#include "l_class_OC_Fifo1_OC_3.h"
class l_class_OC_FifoPong {
  class l_class_OC_Fifo1_OC_3 element1;
  class l_class_OC_Fifo1_OC_3 element2;
  bool pong;
public:
  void run();
  void in_enq(class l_struct_OC_ValuePair *in_enq_v);
  bool in_enq__RDY(void);
  void out_deq(void);
  bool out_deq__RDY(void);
  void out_first(void);
  bool out_first__RDY(void);
};
#endif  // __l_class_OC_FifoPong_H__
