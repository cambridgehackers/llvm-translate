#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo1_OC_1.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector;
extern void l_class_OC_IVector__respond(l_class_OC_IVector *thisp);
extern bool l_class_OC_IVector__respond__RDY(l_class_OC_IVector *thisp);
extern void l_class_OC_IVector__say(l_class_OC_IVector *thisp, BITS6 say_meth, BITS4 say_v);
extern bool l_class_OC_IVector__say__RDY(l_class_OC_IVector *thisp);
class l_class_OC_IVector {
public:
  l_class_OC_Fifo1_OC_1 fifo;
  BITS23 fcounter;
  BITS1 counter;
  BITS10 gcounter;
  l_class_OC_IVectorIndication *ind;
public:
  void run();
  void respond(void) { l_class_OC_IVector__respond(this); }
  bool respond__RDY(void) { return l_class_OC_IVector__respond__RDY(this); }
  void say(BITS6 say_meth, BITS4 say_v) { l_class_OC_IVector__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_IVector__say__RDY(this); }
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
