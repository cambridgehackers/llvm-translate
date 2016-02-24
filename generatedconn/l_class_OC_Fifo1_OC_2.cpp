#include "l_class_OC_Fifo1_OC_2.h"
void l_class_OC_Fifo1_OC_2::in_enq(l_struct_OC_ValueType in_enq_v) {
        element = in_enq_v;
        full = 1;
}
bool l_class_OC_Fifo1_OC_2::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_Fifo1_OC_2::out_deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1_OC_2::out_deq__RDY(void) {
        return full;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_2::out_first(void) {
        l_struct_OC_ValueType out_first;
        return element;
        return out_first;
}
bool l_class_OC_Fifo1_OC_2::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_2::run()
{
}
