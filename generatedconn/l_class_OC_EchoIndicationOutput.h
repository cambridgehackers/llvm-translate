#ifndef __l_class_OC_EchoIndicationOutput_H__
#define __l_class_OC_EchoIndicationOutput_H__
#include "l_class_OC_PipeIn_OC_0.h"
class l_class_OC_EchoIndicationOutput;
extern void l_class_OC_EchoIndicationOutput__heard(l_class_OC_EchoIndicationOutput *thisp, unsigned int heard_meth, unsigned int heard_v);
extern bool l_class_OC_EchoIndicationOutput__heard__RDY(l_class_OC_EchoIndicationOutput *thisp);
class l_class_OC_EchoIndicationOutput : public l_class_OC_EchoIndication {
public:
  l_class_OC_PipeIn_OC_0 *pipe;
public:
  void run();
  void heard(unsigned int heard_meth, unsigned int heard_v) { l_class_OC_EchoIndicationOutput__heard(this, heard_meth, heard_v); }
  bool heard__RDY(void) { return l_class_OC_EchoIndicationOutput__heard__RDY(this); }
  void setpipe(l_class_OC_PipeIn_OC_0 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoIndicationOutput_H__
