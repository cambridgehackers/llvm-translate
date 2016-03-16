#ifndef __l_class_OC_EchoIndicationOutput_H__
#define __l_class_OC_EchoIndicationOutput_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_PipeIn_OC_0.h"
class l_class_OC_EchoIndicationOutput;
extern void l_class_OC_EchoIndicationOutput__heard(void *thisarg, unsigned int heard_meth, unsigned int heard_v);
extern bool l_class_OC_EchoIndicationOutput__heard__RDY(void *thisarg);
class l_class_OC_EchoIndicationOutput {
public:
  l_class_OC_EchoIndication indication;
  l_class_OC_PipeIn_OC_0 *pipe;
public:
  void run();
  void commit();
  l_class_OC_EchoIndicationOutput():
      indication(this, l_class_OC_EchoIndicationOutput__heard__RDY, l_class_OC_EchoIndicationOutput__heard) {
  }
  void setpipe(l_class_OC_PipeIn_OC_0 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoIndicationOutput_H__
