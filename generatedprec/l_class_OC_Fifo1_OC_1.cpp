#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1__in_enq(l_class_OC_Fifo1_OC_1 *thisp, l_struct_OC_ValueType in_enq_v) {
        thisp->full = 1;
        thisp->element.(in_enq_v);
}
bool l_class_OC_Fifo1_OC_1__in_enq__RDY(l_class_OC_Fifo1_OC_1 *thisp) {
        return (thisp->full) ^ 1;
}
void l_class_OC_Fifo1_OC_1__out_deq(l_class_OC_Fifo1_OC_1 *thisp) {
        thisp->full = 0;
}
bool l_class_OC_Fifo1_OC_1__out_deq__RDY(l_class_OC_Fifo1_OC_1 *thisp) {
        return thisp->full;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_1__out_first(l_class_OC_Fifo1_OC_1 *thisp) {
        l_struct_OC_ValueType out_first;
        out_first->(thisp->element);
        return out_first;
}
bool l_class_OC_Fifo1_OC_1__out_first__RDY(l_class_OC_Fifo1_OC_1 *thisp) {
        return thisp->full;
}
void l_class_OC_Fifo1_OC_1::run()
{
}
