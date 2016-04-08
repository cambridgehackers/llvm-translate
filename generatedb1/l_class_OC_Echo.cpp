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
        printf("delay_rule: Echo\n");
}
bool l_class_OC_Echo__delay_rule__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return (((thisp->busy) != 0) & ((thisp->busy_delay) == 0)) != 0;
}
void l_class_OC_Echo__respond_rule(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->busy_delay_shadow = 0;
        thisp->busy_delay_valid = 1;
        printf("respond_rule: Echo\n");
        thisp->indication->heard(thisp->meth_delay, thisp->v_delay);
}
bool l_class_OC_Echo__respond_rule__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return ((thisp->busy_delay) != 0) & tmp__1;
}
void l_class_OC_Echo__say2(void *thisarg, unsigned int say2_meth, unsigned int say2_v) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->meth_temp_shadow = say2_meth;
        thisp->meth_temp_valid = 1;
        thisp->v_temp_shadow = say2_v;
        thisp->v_temp_valid = 1;
        thisp->busy_shadow = 1;
        thisp->busy_valid = 1;
        printf("[%s:%d]Echo\n", ("say2"), 244);
}
bool l_class_OC_Echo__say2__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return ((thisp->busy) != 0) ^ 1;
}
void l_class_OC_Echo__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->meth_temp_shadow = say_meth;
        thisp->meth_temp_valid = 1;
        thisp->v_temp_shadow = say_v;
        thisp->v_temp_valid = 1;
        thisp->busy_shadow = 1;
        thisp->busy_valid = 1;
        printf("[%s:%d]Echo\n", ("say"), 238);
}
bool l_class_OC_Echo__say__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return ((thisp->busy) != 0) ^ 1;
}
void l_class_OC_Echo__x2y(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->y_shadow = thisp->x;
        thisp->y_valid = 1;
        printf("[%s:%d]Echo\n", ("x2y"), 258);
}
bool l_class_OC_Echo__x2y__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return 1;
}
void l_class_OC_Echo__y2x(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        thisp->x_shadow = thisp->y;
        thisp->x_valid = 1;
        printf("[%s:%d]Echo\n", ("y2x"), 250);
}
bool l_class_OC_Echo__y2x__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return 1;
}
void l_class_OC_Echo__y2xnull(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        printf("[%s:%d]Echo\n", ("y2xnull"), 254);
}
bool l_class_OC_Echo__y2xnull__RDY(void *thisarg) {
        l_class_OC_Echo * thisp = (l_class_OC_Echo *)thisarg;
        return 1;
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
    if (x_valid) x = x_shadow;
    x_valid = 0;
    if (y_valid) y = y_shadow;
    y_valid = 0;
}
