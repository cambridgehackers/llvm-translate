#ifndef __l_class_OC_MemServerRequestInput_H__
#define __l_class_OC_MemServerRequestInput_H__
#include "l_class_OC_MemServerRequest.h"
#include "l_class_OC_Pipes.h"
class l_class_OC_MemServerRequestInput {
  l_class_OC_MemServerRequest *request;
  l_class_OC_Pipes pipes;
public:
  void run();
  void setrequest(l_class_OC_MemServerRequest *v) { request = v; }
};
#endif  // __l_class_OC_MemServerRequestInput_H__
