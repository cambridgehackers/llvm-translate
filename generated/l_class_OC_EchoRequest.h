#ifndef __l_class_OC_EchoRequest_H__
#define __l_class_OC_EchoRequest_H__
class l_class_OC_EchoRequest;
extern void l_class_OC_EchoRequest__say(l_class_OC_EchoRequest *thisp, unsigned int say_v);
extern bool l_class_OC_EchoRequest__say__RDY(l_class_OC_EchoRequest *thisp);
class l_class_OC_EchoRequest {
public:
public:
  void run();
  void say(unsigned int say_v) { l_class_OC_EchoRequest__say(this, say_v); }
  bool say__RDY(void) { return l_class_OC_EchoRequest__say__RDY(this); }
};
#endif  // __l_class_OC_EchoRequest_H__
