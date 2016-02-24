#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_EchoRequest.h"
class l_class_OC_Echo : public l_class_OC_EchoRequest {
  l_class_OC_EchoIndication *indication;
public:
  void run();
  virtual void say(unsigned int say_meth, unsigned int say_v);
  virtual bool say__RDY(void);
  void setindication(l_class_OC_EchoIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Echo_H__
