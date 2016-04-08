#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_EchoRequest.h"
class l_class_OC_Echo;
extern void l_class_OC_Echo__delay_rule(void *thisarg);
extern bool l_class_OC_Echo__delay_rule__RDY(void *thisarg);
extern void l_class_OC_Echo__respond_rule(void *thisarg);
extern bool l_class_OC_Echo__respond_rule__RDY(void *thisarg);
extern void l_class_OC_Echo__say2(void *thisarg, unsigned int say2_meth, unsigned int say2_v);
extern bool l_class_OC_Echo__say2__RDY(void *thisarg);
extern void l_class_OC_Echo__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_Echo__say__RDY(void *thisarg);
extern void l_class_OC_Echo__x2y(void *thisarg);
extern bool l_class_OC_Echo__x2y__RDY(void *thisarg);
extern void l_class_OC_Echo__y2x(void *thisarg);
extern bool l_class_OC_Echo__y2x__RDY(void *thisarg);
extern void l_class_OC_Echo__y2xnull(void *thisarg);
extern bool l_class_OC_Echo__y2xnull__RDY(void *thisarg);
class l_class_OC_Echo {
public:
  l_class_OC_EchoRequest request;
  unsigned int busy, busy_shadow; bool busy_valid;
  unsigned int meth_temp, meth_temp_shadow; bool meth_temp_valid;
  unsigned int v_temp, v_temp_shadow; bool v_temp_valid;
  unsigned int busy_delay, busy_delay_shadow; bool busy_delay_valid;
  unsigned int meth_delay, meth_delay_shadow; bool meth_delay_valid;
  unsigned int v_delay, v_delay_shadow; bool v_delay_valid;
  unsigned int x, x_shadow; bool x_valid;
  unsigned int y, y_shadow; bool y_valid;
  l_class_OC_EchoIndication *indication;
public:
  void run();
  void commit();
  l_class_OC_Echo():
      request(this, l_class_OC_Echo__say__RDY, l_class_OC_Echo__say, l_class_OC_Echo__say2__RDY, l_class_OC_Echo__say2) {
  }
  void delay_rule(void) { l_class_OC_Echo__delay_rule(this); }
  bool delay_rule__RDY(void) { return l_class_OC_Echo__delay_rule__RDY(this); }
  void respond_rule(void) { l_class_OC_Echo__respond_rule(this); }
  bool respond_rule__RDY(void) { return l_class_OC_Echo__respond_rule__RDY(this); }
  void x2y(void) { l_class_OC_Echo__x2y(this); }
  bool x2y__RDY(void) { return l_class_OC_Echo__x2y__RDY(this); }
  void y2x(void) { l_class_OC_Echo__y2x(this); }
  bool y2x__RDY(void) { return l_class_OC_Echo__y2x__RDY(this); }
  void y2xnull(void) { l_class_OC_Echo__y2xnull(this); }
  bool y2xnull__RDY(void) { return l_class_OC_Echo__y2xnull__RDY(this); }
  void setindication(l_class_OC_EchoIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Echo_H__
