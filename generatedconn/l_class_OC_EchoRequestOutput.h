#ifndef __l_class_OC_EchoRequestOutput_H__
#define __l_class_OC_EchoRequestOutput_H__
#include "l_class_OC_PipeIn_OC_0.h"
class l_class_OC_EchoRequestOutput {
  l_class_OC_PipeIn_OC_0 *pipe;
public:
  void run();
  void say(unsigned int say_meth, unsigned int say_v);
  bool say__RDY(void);
  void setpipe(l_class_OC_PipeIn_OC_0 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoRequestOutput_H__
