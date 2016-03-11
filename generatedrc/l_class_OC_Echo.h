#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
class l_class_OC_Echo;
extern void l_class_OC_Echo__delay_rule(l_class_OC_Echo *thisp);
extern bool l_class_OC_Echo__delay_rule__RDY(l_class_OC_Echo *thisp);
extern void l_class_OC_Echo__respond_rule(l_class_OC_Echo *thisp);
extern bool l_class_OC_Echo__respond_rule__RDY(l_class_OC_Echo *thisp);
extern void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp);
class l_class_OC_Echo {
public:
  unsigned int busy, busy_shadow; bool busy_valid;
  unsigned int meth_temp, meth_temp_shadow; bool meth_temp_valid;
  unsigned int v_temp, v_temp_shadow; bool v_temp_valid;
  unsigned int busy_delay, busy_delay_shadow; bool busy_delay_valid;
  unsigned int meth_delay, meth_delay_shadow; bool meth_delay_valid;
  unsigned int v_delay, v_delay_shadow; bool v_delay_valid;
  l_class_OC_EchoIndication *indication;
public:
  void run();
  void commit();
  void delay_rule(void) { l_class_OC_Echo__delay_rule(this); }
  bool delay_rule__RDY(void) { return l_class_OC_Echo__delay_rule__RDY(this); }
  void respond_rule(void) { l_class_OC_Echo__respond_rule(this); }
  bool respond_rule__RDY(void) { return l_class_OC_Echo__respond_rule__RDY(this); }
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_Echo__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_Echo__say__RDY(this); }
  void setindication(l_class_OC_EchoIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Echo_H__
