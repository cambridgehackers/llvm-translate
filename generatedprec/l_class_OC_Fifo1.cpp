#include "l_class_OC_Fifo1.h"
void l_class_OC_Fifo1__in_enq(l_class_OC_Fifo1 *thisp, unsigned int in_enq_v) {
        thisp->element_shadow = in_enq_v;
        thisp->element_valid = 1;
        thisp->full_shadow = 1;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1__in_enq__RDY(l_class_OC_Fifo1 *thisp) {
        return (thisp->full) ^ 1;
}
void l_class_OC_Fifo1__out_deq(l_class_OC_Fifo1 *thisp) {
        thisp->full_shadow = 0;
        thisp->full_valid = 1;
}
bool l_class_OC_Fifo1__out_deq__RDY(l_class_OC_Fifo1 *thisp) {
        return thisp->full;
}
unsigned int l_class_OC_Fifo1__out_first(l_class_OC_Fifo1 *thisp) {
        return thisp->element;
}
bool l_class_OC_Fifo1__out_first__RDY(l_class_OC_Fifo1 *thisp) {
        return thisp->full;
}
void l_class_OC_Fifo1::run()
{
    commit();
}
void l_class_OC_Fifo1::commit()
{
    if (element_valid) element = element_shadow;
    element_valid = 0;
    if (full_valid) full = full_shadow;
    full_valid = 0;
}
