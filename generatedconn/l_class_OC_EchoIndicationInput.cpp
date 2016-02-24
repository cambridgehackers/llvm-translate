#include "l_class_OC_EchoIndicationInput.h"
void l_class_OC_EchoIndicationInput::pipe_enq(l_struct_OC_EchoIndication_data pipe_enq_v) {
        if ((pipe_enq_v.tag) == 1)
            request->heard(pipe_enq_v.data.heard.meth, pipe_enq_v.data.heard.v);
}
bool l_class_OC_EchoIndicationInput::pipe_enq__RDY(void) {
        bool tmp__1;
        tmp__1 = request->heard__RDY();
        return tmp__1;
}
void l_class_OC_EchoIndicationInput::run()
{
}
