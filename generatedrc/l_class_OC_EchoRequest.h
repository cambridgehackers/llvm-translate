#ifndef __ER__
#define __ER__
class l_class_OC_EchoRequest {
    void *p;
    GUARDPTR say__RDYp;
    void (*sayp)(void *p, int meth, int v);
 public:
    METHOD(say, (int meth, int v), {return say__RDYp(p); } ) { sayp(p, meth, v); }
    l_class_OC_EchoRequest(void *ap, unsigned long asay__RDYp, unsigned long asayp) {
        p = ap;
        ASSIGNIFCPTR(say);
    }
};
#endif
