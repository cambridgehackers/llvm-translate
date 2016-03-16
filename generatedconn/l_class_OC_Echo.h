#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_EchoRequest.h"
class l_class_OC_Echo;
extern void l_class_OC_Echo__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_Echo__say__RDY(void *thisarg);
class l_class_OC_Echo {
public:
  l_class_OC_EchoRequest request;
  l_class_OC_EchoIndication *indication;
public:
  void run();
  void commit();
  l_class_OC_Echo():
      request(this, l_class_OC_Echo__say__RDY, l_class_OC_Echo__say) {
  }
  void setindication(l_class_OC_EchoIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Echo_H__
