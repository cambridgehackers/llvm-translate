#ifndef __EI__
#define __EI__
class l_class_OC_EchoIndication {
    void *p;
    GUARDPTR heard__RDYp;
    void (*heardp)(void *p, int meth, int v);
 public:
    METHOD(heard, (int meth, int v), {return heard__RDYp(p); } ) { heardp(p, meth, v); }
    l_class_OC_EchoIndication(void *ap, unsigned long aheard__RDYp, unsigned long aheardp) {
        p = ap;
        ASSIGNIFCPTR(heard);
    }
};
#endif
