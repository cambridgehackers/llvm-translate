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
  void respond0(void);
  bool respond0__RDY(void);
  void respond1(void);
  bool respond1__RDY(void);
  void respond2(void);
  bool respond2__RDY(void);
  void respond3(void);
  bool respond3__RDY(void);
  void respond4(void);
  bool respond4__RDY(void);
  void respond5(void);
  bool respond5__RDY(void);
  void respond6(void);
  bool respond6__RDY(void);
  void respond7(void);
  bool respond7__RDY(void);
  void respond8(void);
  bool respond8__RDY(void);
  void respond9(void);
  bool respond9__RDY(void);
  void say(unsigned int say_meth, unsigned int say_v);
  bool say__RDY(void);
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
