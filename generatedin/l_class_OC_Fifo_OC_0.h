#ifndef __l_class_OC_Fifo_OC_0_H__
#define __l_class_OC_Fifo_OC_0_H__
#include "l_class_OC_PipeIn_OC_1.h"
#include "l_class_OC_PipeOut_OC_2.h"
class l_class_OC_Fifo_OC_0;
class l_class_OC_Fifo_OC_0 {
public:
  l_class_OC_PipeIn_OC_1 in;
  l_class_OC_PipeOut_OC_2 out;
public:
  void run();
  void commit();
};
#endif  // __l_class_OC_Fifo_OC_0_H__
