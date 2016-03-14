#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo1_OC_1.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector;
extern void l_class_OC_IVector__respond(void *thisarg);
extern bool l_class_OC_IVector__respond__RDY(void *thisarg);
extern void l_class_OC_IVector__say(void *thisarg, BITS6 say_meth, BITS4 say_v);
extern bool l_class_OC_IVector__say__RDY(void *thisarg);
class l_class_OC_IVector {
public:
  l_class_OC_Fifo1_OC_1 fifo;
  BITS23 fcounter;
  BITS1 counter;
  BITS10 gcounter;
  l_class_OC_IVectorIndication *ind;
public:
  void run();
  void commit();
  void respond(void) { l_class_OC_IVector__respond(this); }
  bool respond__RDY(void) { return l_class_OC_IVector__respond__RDY(this); }
  void say(BITS6 say_meth, BITS4 say_v) { l_class_OC_IVector__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_IVector__say__RDY(this); }
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
