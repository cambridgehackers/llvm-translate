#ifndef __l_class_OC_EchoIndicationOutput_H__
#define __l_class_OC_EchoIndicationOutput_H__
#include "l_class_OC_PipeIn_OC_4.h"
class l_class_OC_EchoIndicationOutput {
  l_class_OC_PipeIn_OC_4 *pipe;
public:
  void run();
  void setpipe(l_class_OC_PipeIn_OC_4 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoIndicationOutput_H__
