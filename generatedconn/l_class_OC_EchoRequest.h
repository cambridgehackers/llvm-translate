#ifndef __l_class_OC_EchoRequest_H__
#define __l_class_OC_EchoRequest_H__
class l_class_OC_EchoRequest {
public:
  void *p;
  bool  (*say__RDYp) (void *);
  void  (*sayp) (void *, unsigned int , unsigned int );
public:
  void say(unsigned int meth, unsigned int v) { sayp(p, meth, v); }
  bool say__RDY(void) { return say__RDYp(p); }
  l_class_OC_EchoRequest(decltype(p) ap, decltype(say__RDYp) asay__RDYp, decltype(sayp) asayp) {
    p = ap;
    say__RDYp = asay__RDYp;
    sayp = asayp;
  }
};
#endif  // __l_class_OC_EchoRequest_H__
