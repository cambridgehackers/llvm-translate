#ifndef __l_class_OC_Echo_H__
#define __l_class_OC_Echo_H__
#include "l_class_OC_EchoIndication.h"
#include "l_class_OC_Fifo1.h"
class l_class_OC_Echo {
public:
  l_class_OC_Fifo1 fifo;
  l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
public:
  void run();
  void respond_rule(void);
  bool respond_rule__RDY(void);
  void say(unsigned int say_v);
  bool say__RDY(void);
  void setind(l_class_OC_EchoIndication *v) { ind = v; }
};
#endif  // __l_class_OC_Echo_H__
