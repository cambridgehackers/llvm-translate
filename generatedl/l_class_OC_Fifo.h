#ifndef __l_class_OC_Fifo_H__
#define __l_class_OC_Fifo_H__
#include "l_class_OC_PipeIn.h"
#include "l_class_OC_PipeOut.h"
class l_class_OC_Fifo;
class l_class_OC_Fifo {
public:
  l_class_OC_PipeIn in;
  l_class_OC_PipeOut out;
public:
  void run();
  void commit();
};
#endif  // __l_class_OC_Fifo_H__
