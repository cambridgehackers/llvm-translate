#include "l_class_OC_EchoIndicationInput.h"
void l_class_OC_EchoIndicationInput::enq(l_struct_OC_EchoIndication_data enq_v) {
        if ((enq_v.tag) == 1)
            request->heard(enq_v.data.heard.meth, enq_v.data.heard.v);
}
bool l_class_OC_EchoIndicationInput::enq__RDY(void) {
        bool tmp__1;
        tmp__1 = request->heard__RDY();
        return tmp__1;
}
void l_class_OC_EchoIndicationInput::run()
{
}
