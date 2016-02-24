#ifndef __l_class_OC_Connect_H__
#define __l_class_OC_Connect_H__
#include "l_class_OC_ConnectIndication.h"
#include "l_class_OC_Echo.h"
#include "l_class_OC_EchoIndicationInput.h"
#include "l_class_OC_EchoIndicationOutput.h"
#include "l_class_OC_EchoRequestInput.h"
#include "l_class_OC_Fifo1_OC_2.h"
class l_class_OC_Connect {
  l_class_OC_Fifo1_OC_2 fifo;
  l_class_OC_ConnectIndication *ind;
  l_class_OC_EchoIndicationOutput lEchoIndicationOutput;
  l_class_OC_EchoRequestInput lEchoRequestInput;
  l_class_OC_Echo lEcho;
  l_class_OC_EchoIndicationInput lEchoIndicationInput_test;
public:
  void run();
  void respond(void);
  bool respond__RDY(void);
  void say(unsigned int say_meth, unsigned int say_v);
  bool say__RDY(void);
  void setind(l_class_OC_ConnectIndication *v) { ind = v; }
};
#endif  // __l_class_OC_Connect_H__
