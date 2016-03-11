#ifndef __l_class_OC_EchoIndicationOutput_H__
#define __l_class_OC_EchoIndicationOutput_H__
#include "l_class_OC_PipeIn_OC_0.h"
#include "l_struct_OC_EchoIndication_data.h"
class l_class_OC_EchoIndicationOutput;
extern void l_class_OC_EchoIndicationOutput__heard(l_class_OC_EchoIndicationOutput *thisp, unsigned int heard_meth, unsigned int heard_v);
extern bool l_class_OC_EchoIndicationOutput__heard__RDY(l_class_OC_EchoIndicationOutput *thisp);
extern void l_class_OC_EchoIndicationOutput__output_rulee(l_class_OC_EchoIndicationOutput *thisp);
extern bool l_class_OC_EchoIndicationOutput__output_rulee__RDY(l_class_OC_EchoIndicationOutput *thisp);
extern void l_class_OC_EchoIndicationOutput__output_ruleo(l_class_OC_EchoIndicationOutput *thisp);
extern bool l_class_OC_EchoIndicationOutput__output_ruleo__RDY(l_class_OC_EchoIndicationOutput *thisp);
class l_class_OC_EchoIndicationOutput {
public:
  l_class_OC_PipeIn_OC_0 *pipe;
  l_struct_OC_EchoIndication_data ind0;
  l_struct_OC_EchoIndication_data ind1;
  unsigned int ind_busy, ind_busy_shadow; bool ind_busy_valid;
  unsigned int even, even_shadow; bool even_valid;
public:
  void run();
  void commit();
  void heard(unsigned int heard_meth, unsigned int heard_v) { l_class_OC_EchoIndicationOutput__heard(this, heard_meth, heard_v); }
  bool heard__RDY(void) { return l_class_OC_EchoIndicationOutput__heard__RDY(this); }
  void output_rulee(void) { l_class_OC_EchoIndicationOutput__output_rulee(this); }
  bool output_rulee__RDY(void) { return l_class_OC_EchoIndicationOutput__output_rulee__RDY(this); }
  void output_ruleo(void) { l_class_OC_EchoIndicationOutput__output_ruleo(this); }
  bool output_ruleo__RDY(void) { return l_class_OC_EchoIndicationOutput__output_ruleo__RDY(this); }
  void setpipe(l_class_OC_PipeIn_OC_0 *v) { pipe = v; }
};
#endif  // __l_class_OC_EchoIndicationOutput_H__
