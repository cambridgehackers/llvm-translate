#include "l_class_OC_Fifo1_OC_3.h"
void l_class_OC_Fifo1_OC_3__in_enq(void *thisarg, l_struct_OC_ValuePair in_enq_v) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        thisp->element = in_enq_v;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_3__in_enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        return (thisp->full) ^ 1;
}
void l_class_OC_Fifo1_OC_3__out_deq(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_3__out_deq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        return thisp->full;
}
l_struct_OC_ValuePair l_class_OC_Fifo1_OC_3__out_first(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        l_struct_OC_ValuePair out_first;
        return thisp->element;
        return out_first;
}
bool l_class_OC_Fifo1_OC_3__out_first__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_3::run()
{
    commit();
}
void l_class_OC_Fifo1_OC_3::commit()
{
    if (full_valid) full = full_shadow;
    full_valid = 0;
}
