#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1__in_enq(void *thisarg, l_struct_OC_ValueType in_enq_v) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
        thisp->element.(in_enq_v);
}
bool l_class_OC_Fifo1_OC_1__in_enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return (thisp->full) ^ 1;
}
void l_class_OC_Fifo1_OC_1__out_deq(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_1__out_deq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        return thisp->full;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__out_first(void *thisarg) {
        l_class_OC_Fifo1_OC_1 * thisp = (l_class_OC_Fifo1_OC_1 *)thisarg;
        l_struct_OC_ValueType out_first;
        out_first->(thisp->element);
        return out_first;
}
bool l_class_OC_Fifo1_OC_1__out_first__RDY(void *thisarg) {
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
