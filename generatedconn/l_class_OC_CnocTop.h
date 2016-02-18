#ifndef __l_class_OC_CnocTop_H__
#define __l_class_OC_CnocTop_H__
#include "l_class_OC_Memread.h"
#include "l_class_OC_MemreadIndicationOutput.h"
#include "l_class_OC_MemreadRequestInput.h"
class l_class_OC_CnocTop {
  l_class_OC_MemreadIndicationOutput lMemreadIndicationOutput;
  l_class_OC_MemreadRequestInput lMemreadRequestInput;
  l_class_OC_Memread lMemread;
public:
  void run();
};
#endif  // __l_class_OC_CnocTop_H__
