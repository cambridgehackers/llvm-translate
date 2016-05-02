#ifndef __l_class_OC_IVectorRequest_H__
#define __l_class_OC_IVectorRequest_H__
class l_class_OC_IVectorRequest;
extern void l_class_OC_IVectorRequest__say(void *thisarg, bool say_meth, bool say_v);
extern bool l_class_OC_IVectorRequest__say__RDY(void *thisarg);
class l_class_OC_IVectorRequest {
public:
public:
  void run();
  void commit();
  void say(bool say_meth, bool say_v) { l_class_OC_IVectorRequest__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_IVectorRequest__say__RDY(this); }
};
#endif  // __l_class_OC_IVectorRequest_H__
