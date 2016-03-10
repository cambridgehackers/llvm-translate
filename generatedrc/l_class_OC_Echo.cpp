#include "l_class_OC_Echo.h"
void l_class_OC_Echo__respond_rule(l_class_OC_Echo *thisp) {
        thisp->busy = 0;
        thisp->indication->heard(thisp->meth_temp, thisp->v_temp);
}
bool l_class_OC_Echo__respond_rule__RDY(l_class_OC_Echo *thisp) {
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return ((thisp->busy) != 0) & tmp__1;
}
void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_meth, unsigned int say_v) {
        thisp->busy = 1;
        thisp->meth_temp = say_meth;
        thisp->v_temp = say_v;
}
bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp) {
        return ((thisp->busy) != 0) ^ 1;
}
void l_class_OC_Echo::run()
{
    if (respond_rule__RDY()) respond_rule();
}
