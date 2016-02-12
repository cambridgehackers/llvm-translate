#include "l_class_OC_Fifo1_OC_4.h"
void l_class_OC_Fifo1_OC_4::in_enq(l_struct_OC_ValuePair in_enq_v) {
        element = in_enq_v;
        full = 1;
}
bool l_class_OC_Fifo1_OC_4::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_Fifo1_OC_4::out_deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1_OC_4::out_deq__RDY(void) {
        return full;
}
l_struct_OC_ValuePair l_class_OC_Fifo1_OC_4::out_first(void) {
        l_struct_OC_ValuePair agg_2e_result;
        return element;
}
bool l_class_OC_Fifo1_OC_4::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_4::run()
{
}
