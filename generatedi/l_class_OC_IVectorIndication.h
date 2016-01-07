#ifndef __l_class_OC_IVectorIndication_H__
#define __l_class_OC_IVectorIndication_H__
class l_class_OC_IVectorIndication {
public:
  void run();
  void heard(l_struct_OC_ValuePair *heard_v);
  bool heard__RDY(void);
};
#endif  // __l_class_OC_IVectorIndication_H__
