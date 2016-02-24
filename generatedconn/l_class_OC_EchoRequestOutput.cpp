#include "l_class_OC_EchoRequestOutput.h"
void l_class_OC_EchoRequestOutput::say(unsigned int say_meth, unsigned int say_v) {
        l_struct_OC_EchoRequest_data ind;
        ind.data.say.meth = say_meth;
        ind.data.say.v = say_v;
        ind.tag = 1;
        pipe->enq(ind);
}
bool l_class_OC_EchoRequestOutput::say__RDY(void) {
        bool tmp__1;
        tmp__1 = pipe->enq__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestOutput::run()
{
}
