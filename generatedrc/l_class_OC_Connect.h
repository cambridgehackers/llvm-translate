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
  l_class_OC_EchoIndicationOutput lEIO;
  l_class_OC_EchoRequestInput lERI;
  l_class_OC_Echo lEcho;
  l_class_OC_EchoRequestOutput lERO_test;
  l_class_OC_EchoIndicationInput lEII_test;
public:
  void run();
  void commit();
  l_class_OC_Connect() {
    lEcho.setindication(&lEIO.ei);
    lERI.setrequest(&lEcho.er);
    lEIO.setpipe(&lEII_test);
    lERO_test.setpipe(&lERI);
   }
};
#endif  // __l_class_OC_Connect_H__
