#include "l_class_OC_EchoRequestInput.h"
void l_class_OC_EchoRequestInput__enq(void *thisarg, l_struct_OC_EchoRequest_data enq_v) {
        l_class_OC_EchoRequestInput * thisp = (l_class_OC_EchoRequestInput *)thisarg;
        printf("entered EchoRequestInput::enq\n");
        if ((enq_v.tag) == 1)
            thisp->request->say(enq_v.data.say.meth, enq_v.data.say.v);
}
bool l_class_OC_EchoRequestInput__enq__RDY(void *thisarg) {
        l_class_OC_EchoRequestInput * thisp = (l_class_OC_EchoRequestInput *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->request->say__RDY();
        return tmp__1;
}
void l_class_OC_EchoRequestInput::run()
{
    commit();
}
void l_class_OC_EchoRequestInput::commit()
{
}
