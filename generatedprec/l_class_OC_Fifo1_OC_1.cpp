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
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
        thisp->element = (enq_v);
}
bool l_class_OC_Fifo1_OC_1__enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return (thisp->full) ^ 1;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__first(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        l_struct_OC_ValueType first;
        first = (thisp->element);
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
