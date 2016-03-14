#ifndef __EI__
#define __EI__
class l_class_OC_EchoIndication {
    void *p;
    GUARDPTR heard__RDYp;
    void (*heardp)(void *p, unsigned int meth, unsigned int v);
 public:
    METHOD(heard, (int meth, int v), {return heard__RDYp(p); } ) { heardp(p, meth, v); }
    l_class_OC_EchoIndication(void *ap, GUARDPTR aheard__RDYp, decltype(heardp) aheardp) {
        p = ap;
        heard__RDYp = aheard__RDYp;
        heardp = aheardp;
    }
};
#endif
