#include "l_class_OC_EchoIndicationOutput.h"
void l_class_OC_EchoIndicationOutput__heard(l_class_OC_EchoIndicationOutput *thisp, unsigned int heard_meth, unsigned int heard_v) {
        l_struct_OC_EchoIndication_data ind;
        ind.data.heard.meth = heard_meth;
        ind.data.heard.v = heard_v;
        ind.tag = 1;
        thisp->pipe->enq(ind);
}
bool l_class_OC_EchoIndicationOutput__heard__RDY(l_class_OC_EchoIndicationOutput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->pipe->enq__RDY();
        return tmp__1;
}
void l_class_OC_EchoIndicationOutput::run()
{
}
