#ifndef __l_class_OC_XsimTop_H__
#define __l_class_OC_XsimTop_H__
#include "l_class_OC_CnocTop.h"
#include "l_class_OC_MMU.h"
#include "l_class_OC_MMUIndicationOutput.h"
#include "l_class_OC_MMURequestInput.h"
#include "l_class_OC_MemServer.h"
#include "l_class_OC_MemServerIndicationOutput.h"
#include "l_class_OC_MemServerRequestInput.h"
class l_class_OC_XsimTop {
  l_class_OC_CnocTop top;
  l_class_OC_MMURequestInput lMMURequestInput;
  l_class_OC_MMUIndicationOutput lMMUIndicationOutput;
  l_class_OC_MemServerIndicationOutput lMemServerIndicationOutput;
  l_class_OC_MemServerRequestInput lMemServerRequestInput;
  l_class_OC_MMU lMMU;
  l_class_OC_MemServer lMemServer;
public:
  void run();
};
#endif  // __l_class_OC_XsimTop_H__
