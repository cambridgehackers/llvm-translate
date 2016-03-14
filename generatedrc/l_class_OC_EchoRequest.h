#ifndef __ER__
#define __ER__
class l_class_OC_EchoRequest {
    void *p;
    GUARDPTR say__RDYp;
    void (*sayp)(void *p, unsigned int meth, unsigned int v);
 public:
    METHOD(say, (int meth, int v), {return say__RDYp(p); } ) { sayp(p, meth, v); }
    l_class_OC_EchoRequest(void *ap, GUARDPTR asay__RDYp, decltype(sayp) asayp) {
        p = ap;
        say__RDYp = asay__RDYp;
        sayp = asayp;
    }
};
#endif
