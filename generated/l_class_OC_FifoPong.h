#ifndef __l_class_OC_FifoPong_H__
#define __l_class_OC_FifoPong_H__
class l_class_OC_FifoPong {
public:
  unsigned int element1;
  unsigned int element2;
  bool pong;
  bool full;
public:
  void run();
  void in_enq(unsigned int in_enq_v);
  bool in_enq__RDY(void);
  void out_deq(void);
  bool out_deq__RDY(void);
  unsigned int out_first(void);
  bool out_first__RDY(void);
};
#endif  // __l_class_OC_FifoPong_H__
