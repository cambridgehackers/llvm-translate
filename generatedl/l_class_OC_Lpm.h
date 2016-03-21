#ifndef __l_class_OC_Lpm_H__
#define __l_class_OC_Lpm_H__
#include "l_class_OC_Fifo1_OC_0.h"
#include "l_class_OC_LpmIndication.h"
#include "l_class_OC_LpmMemory.h"
#include "l_class_OC_LpmRequest.h"
class l_class_OC_Lpm;
extern void l_class_OC_Lpm__enter(void *thisarg);
extern bool l_class_OC_Lpm__enter__RDY(void *thisarg);
extern void l_class_OC_Lpm__exit(void *thisarg);
extern bool l_class_OC_Lpm__exit__RDY(void *thisarg);
extern void l_class_OC_Lpm__recirc(void *thisarg);
extern bool l_class_OC_Lpm__recirc__RDY(void *thisarg);
extern void l_class_OC_Lpm__respond(void *thisarg);
extern bool l_class_OC_Lpm__respond__RDY(void *thisarg);
extern void l_class_OC_Lpm__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_Lpm__say__RDY(void *thisarg);
class l_class_OC_Lpm {
public:
  l_class_OC_Fifo1_OC_0 inQ;
  l_class_OC_Fifo1_OC_0 fifo;
  l_class_OC_Fifo1_OC_0 outQ;
  l_class_OC_LpmMemory mem;
  unsigned int doneCount, doneCount_shadow; bool doneCount_valid;
  l_class_OC_LpmIndication *indication;
public:
  void run();
  void commit();
  void enter(void) { l_class_OC_Lpm__enter(this); }
  bool enter__RDY(void) { return l_class_OC_Lpm__enter__RDY(this); }
  void exit(void) { l_class_OC_Lpm__exit(this); }
  bool exit__RDY(void) { return l_class_OC_Lpm__exit__RDY(this); }
  void recirc(void) { l_class_OC_Lpm__recirc(this); }
  bool recirc__RDY(void) { return l_class_OC_Lpm__recirc__RDY(this); }
  void respond(void) { l_class_OC_Lpm__respond(this); }
  bool respond__RDY(void) { return l_class_OC_Lpm__respond__RDY(this); }
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_Lpm__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_Lpm__say__RDY(this); }
  void setindication(l_class_OC_LpmIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Lpm_H__
