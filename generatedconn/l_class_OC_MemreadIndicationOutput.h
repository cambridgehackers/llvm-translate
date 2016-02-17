#ifndef __l_class_OC_MemreadIndicationOutput_H__
#define __l_class_OC_MemreadIndicationOutput_H__
#include "l_class_OC_PipeIn_OC_6.h"
class l_class_OC_MemreadIndicationOutput {
  l_class_OC_PipeIn_OC_6 *pipe;
public:
  void run();
  void setpipe(l_class_OC_PipeIn_OC_6 *v) { pipe = v; }
};
#endif  // __l_class_OC_MemreadIndicationOutput_H__
