#ifndef __ER__
#define __ER__
class l_class_OC_EchoRequest {
public:
    virtual void say(unsigned int say_meth, unsigned int say_v){};
    virtual bool say__RDY(void) { return true;}
};
#endif
