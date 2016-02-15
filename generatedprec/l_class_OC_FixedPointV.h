#ifndef __l_class_OC_FixedPointV_H__
#define __l_class_OC_FixedPointV_H__
class l_class_OC_FixedPointV {
  unsigned char *data;
  unsigned int size;
public:
  void run();
  void setdata(unsigned char *v) { data = v; }
};
#endif  // __l_class_OC_FixedPointV_H__
