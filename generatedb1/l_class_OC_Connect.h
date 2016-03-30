#ifndef __l_class_OC_Connect_H__
#define __l_class_OC_Connect_H__
#include "l_class_OC_Echo.h"
#include "l_class_OC_EchoIndicationInput.h"
#include "l_class_OC_EchoIndicationOutput.h"
#include "l_class_OC_EchoRequestInput.h"
#include "l_class_OC_EchoRequestOutput.h"
class l_class_OC_Connect;
extern void l_class_OC_Connect__swap2_rule(void *thisarg);
extern bool l_class_OC_Connect__swap2_rule__RDY(void *thisarg);
extern void l_class_OC_Connect__swap_rule(void *thisarg);
extern bool l_class_OC_Connect__swap_rule__RDY(void *thisarg);
class l_class_OC_Connect {
public:
  l_class_OC_EchoIndicationOutput lEIO;
  l_class_OC_EchoRequestInput lERI;
  l_class_OC_Echo lEcho;
  l_class_OC_EchoRequestOutput lERO_test;
  l_class_OC_EchoIndicationInput lEII_test;
public:
  void run();
  void commit();
  l_class_OC_Connect() {
    lERO_test.pipe = &lERI.pipe;
    lEcho.indication = &lEIO.indication;
    lEIO.pipe = &lEII_test.pipe;
    lERI.request = &lEcho.request;
  }
  void swap2_rule(void) { l_class_OC_Connect__swap2_rule(this); }
  bool swap2_rule__RDY(void) { return l_class_OC_Connect__swap2_rule__RDY(this); }
  void swap_rule(void) { l_class_OC_Connect__swap_rule(this); }
  bool swap_rule__RDY(void) { return l_class_OC_Connect__swap_rule__RDY(this); }
};
#endif  // __l_class_OC_Connect_H__
