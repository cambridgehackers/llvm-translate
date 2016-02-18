#ifndef __l_class_OC_Connect_H__
#define __l_class_OC_Connect_H__
#include "l_class_OC_CnocTop.h"
#include "l_class_OC_ConnectIndication.h"
#include "l_class_OC_Fifo1_OC_1.h"
class l_class_OC_Connect {
  l_class_OC_Fifo1_OC_1 fifo;
  BITS23 fcounter;
  BITS1 counter;
  BITS10 gcounter;
  l_class_OC_ConnectIndication *ind;
  l_class_OC_CnocTop top;
public:
  void run();
  void respond(void);
  bool respond__RDY(void);
  void say(BITS6 say_meth, BITS4 say_v);
  bool say__RDY(void);
  void setind(l_class_OC_ConnectIndication *v) { ind = v; }
};
#endif  // __l_class_OC_Connect_H__
