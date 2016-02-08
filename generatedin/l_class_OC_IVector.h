#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_FifoPong.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  l_class_OC_FifoPong fifo0;
  l_class_OC_FifoPong fifo1;
  l_class_OC_FifoPong fifo2;
  l_class_OC_FifoPong fifo3;
  l_class_OC_FifoPong fifo4;
  l_class_OC_FifoPong fifo5;
  l_class_OC_FifoPong fifo6;
  l_class_OC_FifoPong fifo7;
  l_class_OC_FifoPong fifo8;
  l_class_OC_FifoPong fifo9;
  l_class_OC_FifoPong fifo10;
  l_class_OC_IVectorIndication *ind;
  unsigned int vsize;
public:
  void run();
  void respond_rule(void);
  bool respond_rule__RDY(void);
  void say(unsigned int say_meth, unsigned int say_v);
  bool say__RDY(void);
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
