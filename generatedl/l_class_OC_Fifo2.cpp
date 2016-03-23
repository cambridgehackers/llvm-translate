#include "l_class_OC_Fifo2.h"
void l_class_OC_Fifo2__deq(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        thisp->rindex_shadow = ((thisp->rindex) + 1) % 2;
        thisp->rindex_valid = 1;
}
bool l_class_OC_Fifo2__deq__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return (thisp->rindex) != (thisp->windex);
}
void l_class_OC_Fifo2__enq(void *thisarg, l_struct_OC_ValuePair enq_v) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        *((thisp->windex) == 0 ? &thisp->element0:&thisp->element1) = enq_v;
        thisp->windex_shadow = ((thisp->windex) + 1) % 2;
        thisp->windex_valid = 1;
}
bool l_class_OC_Fifo2__enq__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return (((thisp->windex) + 1) % 2) != (thisp->rindex);
}
l_struct_OC_ValuePair l_class_OC_Fifo2__first(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        l_struct_OC_ValuePair first;
        return *((thisp->rindex) == 0 ? &thisp->element0:&thisp->element1);
        return first;
}
bool l_class_OC_Fifo2__first__RDY(void *thisarg) {
        l_class_OC_Fifo2 * thisp = (l_class_OC_Fifo2 *)thisarg;
        return (thisp->rindex) != (thisp->windex);
}
void l_class_OC_Fifo2::run()
{
    commit();
}
void l_class_OC_Fifo2::commit()
{
    if (rindex_valid) rindex = rindex_shadow;
    rindex_valid = 0;
    if (windex_valid) windex = windex_shadow;
    windex_valid = 0;
}
