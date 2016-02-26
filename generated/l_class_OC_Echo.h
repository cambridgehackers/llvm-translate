#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_Fifo1.h"
class l_class_OC_Echo;
extern void l_class_OC_Echo__respond_rule(l_class_OC_Echo *thisp);
extern bool l_class_OC_Echo__respond_rule__RDY(l_class_OC_Echo *thisp);
extern void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_v);
extern bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp);
class l_class_OC_Echo {
public:
  l_class_OC_Fifo1 fifo;
  l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
public:
  void run();
  void respond_rule(void) { l_class_OC_Echo__respond_rule(this); }
  bool respond_rule__RDY(void) { return l_class_OC_Echo__respond_rule__RDY(this); }
  void say(unsigned int say_v) { l_class_OC_Echo__say(this, say_v); }
  bool say__RDY(void) { return l_class_OC_Echo__say__RDY(this); }
  void setind(l_class_OC_EchoIndication *v) { ind = v; }
};
#endif  // __l_class_OC_Echo_H__
