#ifndef __l_class_OC_IVectorIndication_H__
#define __l_class_OC_IVectorIndication_H__
class l_class_OC_IVectorIndication;
extern void l_class_OC_IVectorIndication__heard(l_class_OC_IVectorIndication *thisp, unsigned int heard_meth, unsigned int heard_v);
extern bool l_class_OC_IVectorIndication__heard__RDY(l_class_OC_IVectorIndication *thisp);
class l_class_OC_IVectorIndication {
public:
public:
  void run();
  void commit();
  void heard(unsigned int heard_meth, unsigned int heard_v) { l_class_OC_IVectorIndication__heard(this, heard_meth, heard_v); }
  bool heard__RDY(void) { return l_class_OC_IVectorIndication__heard__RDY(this); }
};
#endif  // __l_class_OC_IVectorIndication_H__
