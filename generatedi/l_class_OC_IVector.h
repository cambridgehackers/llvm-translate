#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_FifoPong.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  class l_class_OC_FifoPong fifo;
  class l_class_OC_IVectorIndication *ind;
  unsigned int pipetemp;
public:
  void run();
  void respond_rule(void);
  bool respond_rule__RDY(void);
  void say(unsigned int say_v);
  bool say__RDY(void);
  void setind(class l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
