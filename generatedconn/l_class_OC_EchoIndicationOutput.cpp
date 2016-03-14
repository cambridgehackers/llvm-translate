#include "l_class_OC_EchoIndicationOutput.h"
void l_class_OC_EchoIndicationOutput__heard(void *thisarg, unsigned int heard_meth, unsigned int heard_v) {
        l_class_OC_EchoIndicationOutput * thisp = (l_class_OC_EchoIndicationOutput *)thisarg;
        l_struct_OC_EchoIndication_data ind;
        ind.tag = 1;
        ind.data.heard.meth = heard_meth;
        ind.data.heard.v = heard_v;
        thisp->pipe->enq(ind);
}
bool l_class_OC_EchoIndicationOutput__heard__RDY(void *thisarg) {
        l_class_OC_EchoIndicationOutput * thisp = (l_class_OC_EchoIndicationOutput *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->pipe->enq__RDY();
        return tmp__1;
}
void l_class_OC_EchoIndicationOutput::run()
{
    commit();
}
void l_class_OC_EchoIndicationOutput::commit()
{
}
