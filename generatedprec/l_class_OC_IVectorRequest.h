#ifndef __l_class_OC_IVectorRequest_H__
#define __l_class_OC_IVectorRequest_H__
class l_class_OC_IVectorRequest {
public:
  void run();
  void say(BITS6 say_meth, BITS4 say_v);
  bool say__RDY(void);
};
#endif  // __l_class_OC_IVectorRequest_H__
