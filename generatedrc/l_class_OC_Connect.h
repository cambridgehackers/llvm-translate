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
  l_class_OC_Connect() {
    lEcho.setindication(&lEIO.indication);
    lERI.setrequest(&lEcho.request);
    lEIO.setpipe(&lEII_test.pipe);
    lERO_test.setpipe(&lERI.pipe);
   }
  void run();
  void commit();
};
#endif  // __l_class_OC_Connect_H__
