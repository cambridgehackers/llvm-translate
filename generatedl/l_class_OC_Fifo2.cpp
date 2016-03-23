#include "l_class_OC_Fifo2.h"
void l_class_OC_Fifo2__deq(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo2__deq__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo2__enq(void *thisarg, l_struct_OC_ValuePair enq_v) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        thisp->element = enq_v;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo2__enq__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return (thisp->full) ^ 1;
}
l_struct_OC_ValuePair l_class_OC_Fifo2__first(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        l_struct_OC_ValuePair first;
        return thisp->element;
        return first;
}
bool l_class_OC_Fifo2__first__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo2::run()
{
    commit();
}
void l_class_OC_Fifo2::commit()
{
    if (full_valid) full = full_shadow;
    full_valid = 0;
}
