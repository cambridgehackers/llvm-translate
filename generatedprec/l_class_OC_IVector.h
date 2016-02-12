#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo_OC_0.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  l_class_OC_Fifo_OC_0 fifo;
  l_class_OC_IVectorIndication *ind;
  unsigned int vsize;
public:
  void run();
  void respond(void);
  bool respond__RDY(void);
  void say(unsigned long long say_meth_2e_coerce, unsigned long long say_v_2e_coerce);
  bool say__RDY(void);
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
