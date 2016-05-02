#include "l_class_OC_Fifo1_OC_3.h"
void l_class_OC_Fifo1_OC_3__deq(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_3__deq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        return thisp->full;
}
void l_class_OC_Fifo1_OC_3__enq(void *thisarg, l_struct_OC_ValuePair enq_v) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        thisp->element = enq_v;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1_OC_3__enq__RDY(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        return (thisp->full) ^ 1;
}
l_struct_OC_ValuePair l_class_OC_Fifo1_OC_3__first(void *thisarg) {
        l_class_OC_Fifo1_OC_3 * thisp = (l_class_OC_Fifo1_OC_3 *)thisarg;
        l_struct_OC_ValuePair retval;
        retval = thisp->element;
        return retval;
}
bool l_class_OC_Fifo1_OC_3__first__RDY(void *thisarg) {
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
