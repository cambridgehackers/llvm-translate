#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1__deq(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_1__deq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_1__enq(void *thisarg, l_struct_OC_ValueType enq_v) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        thisp->element = enq_v;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_1__enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return (thisp->full) ^ 1;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__first(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        unsigned long long call_2e_i_2e_1_2e_i;
        unsigned long long call_2e_i_2e_i;
        l_struct_OC_ValueType first;
        call_2e_i_2e_i = thisp->element.a;
        first.a = call_2e_i_2e_i;
        call_2e_i_2e_1_2e_i = thisp->element.b;
        first.b = call_2e_i_2e_1_2e_i;
        return first;
}
bool l_class_OC_Fifo1_OC_1__first__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_1::run()
{
    commit();
}
void l_class_OC_Fifo1_OC_1::commit()
{
    if (full_valid) full = full_shadow;
    full_valid = 0;
}
