#ifndef __l_class_OC_EchoIndicationOutput_H__
#define __l_class_OC_EchoIndicationOutput_H__
#include "l_class_OC_PipeIn_OC_1.h"
class l_class_OC_EchoIndicationOutput {
  l_class_OC_PipeIn_OC_1 *pipe;
public:
  void run();
  void heard(unsigned int heard_meth, unsigned int heard_v);
  bool heard__RDY(void);
  void setpipe(l_class_OC_PipeIn_OC_1 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoIndicationOutput_H__
