#ifndef __l_class_OC_ConnectIndication_H__
#define __l_class_OC_ConnectIndication_H__
class l_class_OC_ConnectIndication {
public:
  void run();
  void heard(unsigned int heard_meth, unsigned int heard_v);
  bool heard__RDY(void);
};
#endif  // __l_class_OC_ConnectIndication_H__
