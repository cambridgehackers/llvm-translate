#include "l_class_OC_Echo.h"
void l_class_OC_Echo__delay_rule(l_class_OC_Echo *thisp) {
        thisp->busy = 0;
        thisp->busy_delay = 1;
        thisp->meth_delay = thisp->meth_temp;
        thisp->v_delay = thisp->v_temp;
        printf("[%s:%d]Echo\n", ("___ZN4Echo4initEP14EchoIndication_block_invoke_2"), 227);
}
bool l_class_OC_Echo__delay_rule__RDY(l_class_OC_Echo *thisp) {
        return (thisp->busy) != 0;
}
void l_class_OC_Echo__respond_rule(l_class_OC_Echo *thisp) {
        thisp->busy_delay = 0;
        printf("[%s:%d]Echo\n", ("___ZN4Echo4initEP14EchoIndication_block_invoke_4"), 234);
        thisp->indication->heard(thisp->meth_delay, thisp->v_delay);
}
bool l_class_OC_Echo__respond_rule__RDY(l_class_OC_Echo *thisp) {
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return ((thisp->busy_delay) != 0) & tmp__1;
}
void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_meth, unsigned int say_v) {
        thisp->meth_temp = say_meth;
        thisp->v_temp = say_v;
        thisp->busy = 1;
        printf("[%s:%d]Echo\n", ("say"), 217);
}
bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp) {
        return ((thisp->busy) != 0) ^ 1;
}
void l_class_OC_Echo::run()
{
    if (delay_rule__RDY()) delay_rule();
    if (respond_rule__RDY()) respond_rule();
}
