#include "l_class_OC_EchoRequestInput.h"
void l_class_OC_EchoRequestInput__enq(l_class_OC_EchoRequestInput *thisp, l_struct_OC_EchoRequest_data enq_v) {
        printf("entered EchoRequestInput::enq tag %d\n", enq_v.tag);
        if ((enq_v.tag) == 1)
            thisp->request->say(enq_v.data.say.meth, enq_v.data.say.v);
}
bool l_class_OC_EchoRequestInput__enq__RDY(l_class_OC_EchoRequestInput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->request->say__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestInput::run()
{
}
