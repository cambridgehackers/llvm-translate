#ifndef __l_class_OC_IVectorRequest_H__
#define __l_class_OC_IVectorRequest_H__
class l_class_OC_IVectorRequest;
extern void l_class_OC_IVectorRequest__say(l_class_OC_IVectorRequest *thisp, BITS6 say_meth, BITS4 say_v);
extern bool l_class_OC_IVectorRequest__say__RDY(l_class_OC_IVectorRequest *thisp);
class l_class_OC_IVectorRequest {
public:
public:
  void run();
  void commit();
  void say(BITS6 say_meth, BITS4 say_v) { l_class_OC_IVectorRequest__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_IVectorRequest__say__RDY(this); }
};
#endif  // __l_class_OC_IVectorRequest_H__
