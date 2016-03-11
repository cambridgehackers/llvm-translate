#include "l_class_OC_EchoIndicationInput.h"
void l_class_OC_EchoIndicationInput__enq(l_class_OC_EchoIndicationInput *thisp, l_struct_OC_EchoIndication_data enq_v) {
        if ((enq_v.tag) == 1)
            thisp->meth_delay = enq_v.data.heard.meth;
        if ((enq_v.tag) == 1)
            thisp->v_delay = enq_v.data.heard.v;
        if ((enq_v.tag) == 1)
            thisp->busy_delay = 1;
        printf("[%s:%d]EchoIndicationInput tag %d\n", ("enq"), 185, enq_v.tag);
}
bool l_class_OC_EchoIndicationInput__enq__RDY(l_class_OC_EchoIndicationInput *thisp) {
        return ((thisp->busy_delay) != 0) ^ 1;
}
void l_class_OC_EchoIndicationInput__input_rule(l_class_OC_EchoIndicationInput *thisp) {
        thisp->busy_delay = 0;
        printf("[input_rule:%d]EchoIndicationInput\n", 199);
        thisp->request->heard(thisp->meth_delay, thisp->v_delay);
}
bool l_class_OC_EchoIndicationInput__input_rule__RDY(l_class_OC_EchoIndicationInput *thisp) {
        bool tmp__1;
        tmp__1 = thisp->request->heard__RDY();
        return ((thisp->busy_delay) != 0) & tmp__1;
}
void l_class_OC_EchoIndicationInput::run()
{
    if (input_rule__RDY()) input_rule();
}
