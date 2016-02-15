#ifndef __l_class_OC_IVector_H__
#define __l_class_OC_IVector_H__
#include "l_class_OC_Fifo_OC_1.h"
#include "l_class_OC_FixedPointV.h"
#include "l_class_OC_IVectorIndication.h"
class l_class_OC_IVector {
  l_class_OC_Fifo_OC_1 fifo;
  l_class_OC_FixedPointV counter;
  l_class_OC_FixedPointV gcounter;
  l_class_OC_IVectorIndication *ind;
public:
  void run();
  void respond(void);
  bool respond__RDY(void);
  void say(bool say_meth_2e_coerce, bool say_v_2e_coerce);
  bool say__RDY(void);
  void setind(l_class_OC_IVectorIndication *v) { ind = v; }
};
#endif  // __l_class_OC_IVector_H__
