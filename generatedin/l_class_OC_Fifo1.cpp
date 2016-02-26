#include "l_class_OC_Fifo1.h"
void l_class_OC_Fifo1__in_enq(l_class_OC_Fifo1 *thisp, unsigned int in_enq_v) {
        thisp->element = in_enq_v;
        thisp->full = 1;
}
bool l_class_OC_Fifo1__in_enq__RDY(l_class_OC_Fifo1 *thisp) {
        return (thisp->full) ^ 1;
}
void l_class_OC_Fifo1__out_deq(l_class_OC_Fifo1 *thisp) {
        thisp->full = 0;
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
}
