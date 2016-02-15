#ifndef __l_class_OC_FixedPoint_H__
#define __l_class_OC_FixedPoint_H__
class l_class_OC_FixedPoint {
  unsigned char *data;
public:
  void run();
  void setdata(unsigned char *v) { data = v; }
};
#endif  // __l_class_OC_FixedPoint_H__
