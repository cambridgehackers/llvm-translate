#include "l_class_OC_EchoRequestInput.h"
void l_class_OC_EchoRequestInput::pipe_enq(l_struct_OC_EchoRequest_data pipe_enq_v) {
        if ((pipe_enq_v.tag) == 1)
            request->say(pipe_enq_v.data.say.meth, pipe_enq_v.data.say.v);
}
bool l_class_OC_EchoRequestInput::pipe_enq__RDY(void) {
        bool tmp__1;
        tmp__1 = request->say__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestInput::run()
{
}
