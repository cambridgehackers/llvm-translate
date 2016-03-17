#ifndef __l_class_OC_Fifo_OC_2_H__
#define __l_class_OC_Fifo_OC_2_H__
#include "l_class_OC_PipeIn_OC_3.h"
#include "l_class_OC_PipeOut_OC_4.h"
class l_class_OC_Fifo_OC_2;
class l_class_OC_Fifo_OC_2 {
public:
  l_class_OC_PipeIn_OC_3 in;
  l_class_OC_PipeOut_OC_4 out;
public:
  void run();
  void commit();
};
#endif  // __l_class_OC_Fifo_OC_2_H__
