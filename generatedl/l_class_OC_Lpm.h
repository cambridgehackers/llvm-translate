#ifndef __l_class_OC_Lpm_H__
#define __l_class_OC_Lpm_H__
#include "l_class_OC_Fifo1_OC_0.h"
#include "l_class_OC_LpmIndication.h"
#include "l_class_OC_LpmRequest.h"
class l_class_OC_Lpm;
extern void l_class_OC_Lpm__respond(void *thisarg);
extern bool l_class_OC_Lpm__respond__RDY(void *thisarg);
extern void l_class_OC_Lpm__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_Lpm__say__RDY(void *thisarg);
class l_class_OC_Lpm {
public:
  l_class_OC_Fifo1_OC_0 fifo;
  l_class_OC_LpmIndication *indication;
public:
  void run();
  void commit();
  void respond(void) { l_class_OC_Lpm__respond(this); }
  bool respond__RDY(void) { return l_class_OC_Lpm__respond__RDY(this); }
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_Lpm__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_Lpm__say__RDY(this); }
  void setindication(l_class_OC_LpmIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Lpm_H__
