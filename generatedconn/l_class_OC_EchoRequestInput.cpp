#include "l_class_OC_EchoRequestInput.h"
void l_class_OC_EchoRequestInput::enq(l_struct_OC_EchoRequest_data enq_v) {
        printf("entered EchoRequestInput::enq %p\n", request);
        if ((enq_v.tag) == 1)
            request->say(enq_v.data.say.meth, enq_v.data.say.v);
}
bool l_class_OC_EchoRequestInput::enq__RDY(void) {
        bool tmp__1;
        tmp__1 = request->say__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestInput::run()
{
}
