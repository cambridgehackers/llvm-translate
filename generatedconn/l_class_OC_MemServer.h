#ifndef __l_class_OC_MemServer_H__
#define __l_class_OC_MemServer_H__
#include "l_class_OC_MemServerRequest.h"
class l_class_OC_MemServer {
  l_class_OC_MemServerRequest request;
public:
  void run();
};
#endif  // __l_class_OC_MemServer_H__
