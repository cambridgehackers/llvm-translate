#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo1_OC_1.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  l_class_OC_Fifo1_OC_1 fifo;
  l_class_OC_IVectorIndication *ind;
public:
  void run();
  void respond(void);
  bool respond__RDY(void);
  void say(BITS6 say_meth, BITS4 say_v);
  bool say__RDY(void);
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
