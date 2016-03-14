#ifndef __l_class_OC_EchoRequestOutput_H__
#define __l_class_OC_EchoRequestOutput_H__
#include "l_class_OC_PipeIn.h"
class l_class_OC_EchoRequestOutput;
extern void l_class_OC_EchoRequestOutput__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_EchoRequestOutput__say__RDY(void *thisarg);
class l_class_OC_EchoRequestOutput {
public:
  l_class_OC_PipeIn *pipe;
public:
  void run();
  void commit();
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_EchoRequestOutput__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_EchoRequestOutput__say__RDY(this); }
  void setpipe(l_class_OC_PipeIn *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoRequestOutput_H__
