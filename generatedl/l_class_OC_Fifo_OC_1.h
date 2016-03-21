#ifndef __l_class_OC_Fifo_OC_1_H__
#define __l_class_OC_Fifo_OC_1_H__
#include "l_class_OC_PipeIn_OC_2.h"
#include "l_class_OC_PipeOut_OC_3.h"
class l_class_OC_Fifo_OC_1;
class l_class_OC_Fifo_OC_1 {
public:
  l_class_OC_PipeIn_OC_2 in;
  l_class_OC_PipeOut_OC_3 out;
public:
  void run();
  void commit();
};
#endif  // __l_class_OC_Fifo_OC_1_H__
