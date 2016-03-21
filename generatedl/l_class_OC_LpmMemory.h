#ifndef __l_class_OC_LpmMemory_H__
#define __l_class_OC_LpmMemory_H__
#include "l_struct_OC_ValuePair.h"
class l_class_OC_LpmMemory;
extern void l_class_OC_LpmMemory__memdelay(void *thisarg);
extern bool l_class_OC_LpmMemory__memdelay__RDY(void *thisarg);
extern void l_class_OC_LpmMemory__req(void *thisarg, l_struct_OC_ValuePair v);
extern bool l_class_OC_LpmMemory__req__RDY(void *thisarg);
extern void l_class_OC_LpmMemory__resAccept(void *thisarg);
extern bool l_class_OC_LpmMemory__resAccept__RDY(void *thisarg);
extern l_struct_OC_ValuePair l_class_OC_LpmMemory__resValue(void *thisarg);
extern bool l_class_OC_LpmMemory__resValue__RDY(void *thisarg);
class l_class_OC_LpmMemory {
public:
  unsigned int delayCount, delayCount_shadow; bool delayCount_valid;
  l_struct_OC_ValuePair saved;
public:
  void run();
  void commit();
  void memdelay(void) { l_class_OC_LpmMemory__memdelay(this); }
  bool memdelay__RDY(void) { return l_class_OC_LpmMemory__memdelay__RDY(this); }
  void req(l_struct_OC_ValuePair v) { l_class_OC_LpmMemory__req(this, v); }
  bool req__RDY(void) { return l_class_OC_LpmMemory__req__RDY(this); }
  void resAccept(void) { l_class_OC_LpmMemory__resAccept(this); }
  bool resAccept__RDY(void) { return l_class_OC_LpmMemory__resAccept__RDY(this); }
  l_struct_OC_ValuePair resValue(void) { return l_class_OC_LpmMemory__resValue(this); }
  bool resValue__RDY(void) { return l_class_OC_LpmMemory__resValue__RDY(this); }
};
#endif  // __l_class_OC_LpmMemory_H__
