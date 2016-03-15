#include "l_class_OC_Echo.h"
void l_class_OC_Echo__delay_rule(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->busy_shadow = 0;
        thisp->busy_valid = 1;
        thisp->busy_delay_shadow = 1;
        thisp->busy_delay_valid = 1;
        thisp->meth_delay_shadow = thisp->meth_temp;
        thisp->meth_delay_valid = 1;
        thisp->v_delay_shadow = thisp->v_temp;
        thisp->v_delay_valid = 1;
        printf("[delay_rule:%d]Echo\n", 222);
}
bool l_class_OC_Echo__delay_rule__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return (((thisp->busy) != 0) & ((thisp->busy_delay) == 0)) != 0;
}
void l_class_OC_Echo__respond_rule(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->busy_delay_shadow = 0;
        thisp->busy_delay_valid = 1;
        printf("[respond_rule:%d]Echo\n", 229);
        thisp->indication->heard(thisp->meth_delay, thisp->v_delay);
}
bool l_class_OC_Echo__respond_rule__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return ((thisp->busy_delay) != 0) & tmp__1;
}
void l_class_OC_Echo__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->meth_temp_shadow = say_meth;
        thisp->meth_temp_valid = 1;
        thisp->v_temp_shadow = say_v;
        thisp->v_temp_valid = 1;
        thisp->busy_shadow = 1;
        thisp->busy_valid = 1;
        printf("[%s:%d]Echo\n", ("say"), 213);
}
bool l_class_OC_Echo__say__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return ((thisp->busy) != 0) ^ 1;
}
void l_class_OC_Echo::run()
{
    if (delay_rule__RDY()) delay_rule();
    if (respond_rule__RDY()) respond_rule();
    commit();
}
void l_class_OC_Echo::commit()
{
    if (busy_valid) busy = busy_shadow;
    busy_valid = 0;
    if (meth_temp_valid) meth_temp = meth_temp_shadow;
    meth_temp_valid = 0;
    if (v_temp_valid) v_temp = v_temp_shadow;
    v_temp_valid = 0;
    if (busy_delay_valid) busy_delay = busy_delay_shadow;
    busy_delay_valid = 0;
    if (meth_delay_valid) meth_delay = meth_delay_shadow;
    meth_delay_valid = 0;
    if (v_delay_valid) v_delay = v_delay_shadow;
    v_delay_valid = 0;
}
