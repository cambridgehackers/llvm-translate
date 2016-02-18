#ifndef __l_class_OC_Memread_H__
#define __l_class_OC_Memread_H__
#include "l_class_OC_MemreadIndication.h"
class l_class_OC_Memread {
  l_class_OC_MemreadIndication *indication;
public:
  void run();
  void setindication(l_class_OC_MemreadIndication *v) { indication = v; }
};
#endif  // __l_class_OC_Memread_H__
