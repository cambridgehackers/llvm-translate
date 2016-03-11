#ifndef __l_class_OC_EchoIndication_H__
#define __l_class_OC_EchoIndication_H__
class l_class_OC_EchoIndication;
extern void l_class_OC_EchoIndication__heard(l_class_OC_EchoIndication *thisp, unsigned int heard_v);
extern bool l_class_OC_EchoIndication__heard__RDY(l_class_OC_EchoIndication *thisp);
class l_class_OC_EchoIndication {
public:
public:
  void run();
  void commit();
  void heard(unsigned int heard_v) { l_class_OC_EchoIndication__heard(this, heard_v); }
  bool heard__RDY(void) { return l_class_OC_EchoIndication__heard__RDY(this); }
};
#endif  // __l_class_OC_EchoIndication_H__
