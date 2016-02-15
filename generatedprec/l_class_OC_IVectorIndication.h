#ifndef __l_class_OC_IVectorIndication_H__
#define __l_class_OC_IVectorIndication_H__
class l_class_OC_IVectorIndication {
public:
  void run();
  void heard(BITS6 heard_meth, BITS4 heard_v);
  bool heard__RDY(void);
};
#endif  // __l_class_OC_IVectorIndication_H__
