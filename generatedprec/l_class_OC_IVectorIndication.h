#ifndef __l_class_OC_IVectorIndication_H__
#define __l_class_OC_IVectorIndication_H__
class l_class_OC_IVectorIndication;
extern void l_class_OC_IVectorIndication__heard(l_class_OC_IVectorIndication *thisp, BITS6 heard_meth, BITS4 heard_v);
extern bool l_class_OC_IVectorIndication__heard__RDY(l_class_OC_IVectorIndication *thisp);
class l_class_OC_IVectorIndication {
public:
public:
  void run();
  void heard(BITS6 heard_meth, BITS4 heard_v) { l_class_OC_IVectorIndication__heard(this, heard_meth, heard_v); }
  bool heard__RDY(void) { return l_class_OC_IVectorIndication__heard__RDY(this); }
};
#endif  // __l_class_OC_IVectorIndication_H__
