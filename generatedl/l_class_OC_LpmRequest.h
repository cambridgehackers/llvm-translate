#ifndef __l_class_OC_LpmRequest_H__
#define __l_class_OC_LpmRequest_H__
class l_class_OC_LpmRequest;
extern void l_class_OC_LpmRequest__say(void *thisarg, unsigned int say_meth, unsigned int say_v);
extern bool l_class_OC_LpmRequest__say__RDY(void *thisarg);
class l_class_OC_LpmRequest {
public:
public:
  void run();
  void commit();
  void say(unsigned int say_meth, unsigned int say_v) { l_class_OC_LpmRequest__say(this, say_meth, say_v); }
  bool say__RDY(void) { return l_class_OC_LpmRequest__say__RDY(this); }
};
#endif  // __l_class_OC_LpmRequest_H__