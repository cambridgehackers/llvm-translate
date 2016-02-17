#ifndef __l_class_OC_Memread_H__
#define __l_class_OC_Memread_H__
#include "l_class_OC_MemReadClient.h"
class l_class_OC_Memread {
  l_class_OC_MemReadClient *readers;
public:
  void run();
  void setreaders(l_class_OC_MemReadClient *v) { readers = v; }
};
#endif  // __l_class_OC_Memread_H__
