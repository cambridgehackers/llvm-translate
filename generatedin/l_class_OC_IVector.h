#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo_OC_0.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  l_class_OC_Fifo_OC_0 *fifo;
  l_class_OC_IVectorIndication *ind;
  unsigned int vsize;
public:
  void run();
  void respond_rule(void);
  bool respond_rule__RDY(void);
  void say(unsigned int say_meth, unsigned int say_v);
  bool say__RDY(void);
  void setfifo(l_class_OC_Fifo_OC_0 *v) { fifo = v; }
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
