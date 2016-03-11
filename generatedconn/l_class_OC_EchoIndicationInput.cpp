#include "l_class_OC_EchoIndicationInput.h"
void l_class_OC_EchoIndicationInput__enq(l_class_OC_EchoIndicationInput *thisp, l_struct_OC_EchoIndication_data enq_v) {
        if ((enq_v.tag) == 1)
            thisp->request->heard(enq_v.data.heard.meth, enq_v.data.heard.v);
}
bool l_class_OC_EchoIndicationInput__enq__RDY(l_class_OC_EchoIndicationInput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->request->heard__RDY();
        return tmp__1;
}
void l_class_OC_EchoIndicationInput::run()
{
    commit();
}
void l_class_OC_EchoIndicationInput::commit()
{
}
