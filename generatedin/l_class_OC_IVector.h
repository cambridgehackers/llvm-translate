#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_FifoPong.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector;
extern void l_class_OC_IVector__respond0(void *thisarg);
extern bool l_class_OC_IVector__respond0__RDY(void *thisarg);
extern void l_class_OC_IVector__respond1(void *thisarg);
extern bool l_class_OC_IVector__respond1__RDY(void *thisarg);
extern void l_class_OC_IVector__respond2(void *thisarg);
extern bool l_class_OC_IVector__respond2__RDY(void *thisarg);
extern void l_class_OC_IVector__respond3(void *thisarg);
extern bool l_class_OC_IVector__respond3__RDY(void *thisarg);
extern void l_class_OC_IVector__respond4(void *thisarg);
extern bool l_class_OC_IVector__respond4__RDY(void *thisarg);
extern void l_class_OC_IVector__respond5(void *thisarg);
extern bool l_class_OC_IVector__respond5__RDY(void *thisarg);
extern void l_class_OC_IVector__respond6(void *thisarg);
extern bool l_class_OC_IVector__respond6__RDY(void *thisarg);
extern void l_class_OC_IVector__respond7(void *thisarg);
extern bool l_class_OC_IVector__respond7__RDY(void *thisarg);
extern void l_class_OC_IVector__respond8(void *thisarg);
extern bool l_class_OC_IVector__respond8__RDY(void *thisarg);
extern void l_class_OC_IVector__respond9(void *thisarg);
extern bool l_class_OC_IVector__respond9__RDY(void *thisarg);
extern void l_class_OC_IVector__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_IVector__say__RDY(void *thisarg);
class l_class_OC_IVector {
public:
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
  unsigned int vsize, vsize_shadow; bool vsize_valid;
public:
  void run();
  void commit();
  void respond0(void) { l_class_OC_IVector__respond0(this); }
  bool respond0__RDY(void) { return l_class_OC_IVector__respond0__RDY(this); }
  void respond1(void) { l_class_OC_IVector__respond1(this); }
  bool respond1__RDY(void) { return l_class_OC_IVector__respond1__RDY(this); }
  void respond2(void) { l_class_OC_IVector__respond2(this); }
  bool respond2__RDY(void) { return l_class_OC_IVector__respond2__RDY(this); }
  void respond3(void) { l_class_OC_IVector__respond3(this); }
  bool respond3__RDY(void) { return l_class_OC_IVector__respond3__RDY(this); }
  void respond4(void) { l_class_OC_IVector__respond4(this); }
  bool respond4__RDY(void) { return l_class_OC_IVector__respond4__RDY(this); }
  void respond5(void) { l_class_OC_IVector__respond5(this); }
  bool respond5__RDY(void) { return l_class_OC_IVector__respond5__RDY(this); }
  void respond6(void) { l_class_OC_IVector__respond6(this); }
  bool respond6__RDY(void) { return l_class_OC_IVector__respond6__RDY(this); }
  void respond7(void) { l_class_OC_IVector__respond7(this); }
  bool respond7__RDY(void) { return l_class_OC_IVector__respond7__RDY(this); }
  void respond8(void) { l_class_OC_IVector__respond8(this); }
  bool respond8__RDY(void) { return l_class_OC_IVector__respond8__RDY(this); }
  void respond9(void) { l_class_OC_IVector__respond9(this); }
  bool respond9__RDY(void) { return l_class_OC_IVector__respond9__RDY(this); }
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_IVector__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_IVector__say__RDY(this); }
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
