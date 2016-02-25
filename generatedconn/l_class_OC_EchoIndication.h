#ifndef __EI__
#define __EI__
class l_class_OC_EchoIndication {
public:
    virtual void heard(unsigned int heard_meth, unsigned int heard_v) {};
    virtual bool heard__RDY(void) { return true;}
};
#endif
