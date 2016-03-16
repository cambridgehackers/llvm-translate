#ifndef __l_class_OC_PipeOut_H__
#define __l_class_OC_PipeOut_H__
class l_class_OC_PipeOut {
public:
  void *p;
  bool  (*deq__RDYp) (void *);
  void  (*deqp) (void *);
  bool  (*first__RDYp) (void *);
  unsigned int  (*firstp) (void *);
public:
  void deq(void) { deqp(p); }
  bool deq__RDY(void) { return deq__RDYp(p); }
  unsigned int first(void) { return firstp(p); }
  bool first__RDY(void) { return first__RDYp(p); }
  l_class_OC_PipeOut(decltype(p) ap, decltype(deq__RDYp) adeq__RDYp, decltype(deqp) adeqp, decltype(first__RDYp) afirst__RDYp, decltype(firstp) afirstp) {
    p = ap;
    deq__RDYp = adeq__RDYp;
    deqp = adeqp;
    first__RDYp = afirst__RDYp;
    firstp = afirstp;
  }
};
#endif  // __l_class_OC_PipeOut_H__
