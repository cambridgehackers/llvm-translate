#include "l_class_OC_LpmMemory.h"
void l_class_OC_LpmMemory__memdelay(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        thisp->delayCount_shadow = (thisp->delayCount) - 1;
        thisp->delayCount_valid = 1;
}
bool l_class_OC_LpmMemory__memdelay__RDY(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        return (thisp->delayCount) > 1;
}
void l_class_OC_LpmMemory__req(void *thisarg, l_struct_OC_ValuePair v) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        thisp->delayCount_shadow = 4;
        thisp->delayCount_valid = 1;
        thisp->saved = v;
}
bool l_class_OC_LpmMemory__req__RDY(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        return (thisp->delayCount) == 0;
}
void l_class_OC_LpmMemory__resAccept(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        thisp->delayCount_shadow = 0;
        thisp->delayCount_valid = 1;
}
bool l_class_OC_LpmMemory__resAccept__RDY(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        return (thisp->delayCount) == 1;
}
l_struct_OC_ValuePair l_class_OC_LpmMemory__resValue(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        l_struct_OC_ValuePair agg_2e_result;
        return thisp->saved;
        return agg_2e_result;
}
bool l_class_OC_LpmMemory__resValue__RDY(void *thisarg) {
        l_class_OC_LpmMemory * thisp = (l_class_OC_LpmMemory *)thisarg;
        return (thisp->delayCount) == 1;
}
void l_class_OC_LpmMemory::run()
{
    if (memdelay__RDY()) memdelay();
    commit();
}
void l_class_OC_LpmMemory::commit()
{
    if (delayCount_valid) delayCount = delayCount_shadow;
    delayCount_valid = 0;
}
