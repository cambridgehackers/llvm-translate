#ifndef __l_class_OC_Fifo_H__
#define __l_class_OC_Fifo_H__
class l_class_OC_Fifo;
extern void l_class_OC_Fifo__in_enq(void *thisarg, unsigned int in_enq_v);
extern bool l_class_OC_Fifo__in_enq__RDY(void *thisarg);
extern void l_class_OC_Fifo__out_deq(void *thisarg);
extern bool l_class_OC_Fifo__out_deq__RDY(void *thisarg);
extern unsigned int l_class_OC_Fifo__out_first(void *thisarg);
extern bool l_class_OC_Fifo__out_first__RDY(void *thisarg);
class l_class_OC_Fifo {
public:
public:
  void run();
  void commit();
  void in_enq(unsigned int in_enq_v) { l_class_OC_Fifo__in_enq(this, in_enq_v); }
  bool in_enq__RDY(void) { return l_class_OC_Fifo__in_enq__RDY(this); }
  void out_deq(void) { l_class_OC_Fifo__out_deq(this); }
  bool out_deq__RDY(void) { return l_class_OC_Fifo__out_deq__RDY(this); }
  unsigned int out_first(void) { return l_class_OC_Fifo__out_first(this); }
  bool out_first__RDY(void) { return l_class_OC_Fifo__out_first__RDY(this); }
};
#endif  // __l_class_OC_Fifo_H__
