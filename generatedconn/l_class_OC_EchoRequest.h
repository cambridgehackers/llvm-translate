#ifndef __ER__
#define __ER__
class l_class_OC_EchoRequest {
    void *p;
    GUARDPTR say__RDYp;
    void (*sayp)(void *p, int meth, int v);
 public:
    METHOD(say, (int meth, int v), {return true; } ) { sayp(p, meth, v); }
    void init(void *ap, unsigned long asay__RDYp, unsigned long asayp) {
        p = ap;
        ASSIGNIFCPTR(say);
    }
    l_class_OC_EchoRequest(): p(NULL), say__RDYp(NULL), sayp(NULL) { }
};
#endif
