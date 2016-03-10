#ifndef __l_class_OC_Connect_H__
#define __l_class_OC_Connect_H__
#include "l_class_OC_Echo.h"
#include "l_class_OC_EchoIndicationInput.h"
#include "l_class_OC_EchoIndicationOutput.h"
#include "l_class_OC_EchoRequestInput.h"
#include "l_class_OC_EchoRequestOutput.h"
class l_class_OC_Connect;
class l_class_OC_Connect {
public:
  l_class_OC_EchoIndicationOutput lEchoIndicationOutput;
  l_class_OC_EchoRequestInput lEchoRequestInput;
  l_class_OC_Echo lEcho;
  l_class_OC_EchoRequestOutput lEchoRequestOutput_test;
  l_class_OC_EchoIndicationInput lEchoIndicationInput_test;
public:
  void run();
};
#endif  // __l_class_OC_Connect_H__
