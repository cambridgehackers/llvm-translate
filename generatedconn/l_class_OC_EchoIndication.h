#ifndef __EI__
#define __EI__
class l_class_OC_EchoIndication {
    void *p;
    GUARDPTR heard__RDYp;
    void (*heardp)(void *p, int meth, int v);
 public:
    METHOD(heard, (int meth, int v), {return true; } ) { heardp(p, meth, v); }
    void init(void *ap, unsigned long aheard__RDYp, unsigned long aheardp) {
        p = ap;
        ASSIGNIFCPTR(heard);
    }
    l_class_OC_EchoIndication(): p(NULL), heard__RDYp(NULL), heardp(NULL) { }
};
#endif
