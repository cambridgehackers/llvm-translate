#include "l_class_OC_EchoRequestOutput.h"
void l_class_OC_EchoRequestOutput__say(l_class_OC_EchoRequestOutput *thisp, unsigned int say_meth, unsigned int say_v) {
        l_struct_OC_EchoRequest_data ind;
        ind.tag = 1;
        ind.data.say.meth = say_meth;
        ind.data.say.v = say_v;
        printf("entered EchoRequestOutput::say\n");
        thisp->pipe->enq(ind);
}
bool l_class_OC_EchoRequestOutput__say__RDY(l_class_OC_EchoRequestOutput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->pipe->enq__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestOutput::run()
{
    commit();
}
void l_class_OC_EchoRequestOutput::commit()
{
}
