#include "l_class_OC_Fifo1_OC_0.h"
void l_class_OC_Fifo1_OC_0__deq(void *thisarg) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_0__deq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_0__enq(void *thisarg, l_struct_OC_ValueType enq_v) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        thisp->element = enq_v;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_0__enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        return (thisp->full) ^ 1;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_0__first(void *thisarg) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        l_struct_OC_ValueType retval;
        retval = thisp->element;
        return retval;
}
bool l_class_OC_Fifo1_OC_0__first__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_0 * thisp = (l_class_OC_Fifo1_OC_0 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_0::run()
{
    commit();
}
void l_class_OC_Fifo1_OC_0::commit()
{
    if (full_valid) full = full_shadow;
    full_valid = 0;
}
