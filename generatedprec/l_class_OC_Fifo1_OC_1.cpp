#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1::in_enq(l_struct_OC_ValueType in_enq_v) {
        element.a.data = (in_enq_v).a.data;
        element.b.data = (in_enq_v).b.data;
        full = 1;
}
bool l_class_OC_Fifo1_OC_1::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_Fifo1_OC_1::out_deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1_OC_1::out_deq__RDY(void) {
        return full;
}
l_struct_OC_ValueType l_class_OC_Fifo1_OC_1::out_first(void) {
        l_struct_OC_ValueType agg_2e_result;
        (agg_2e_result).a.data = element.a.data;
        (agg_2e_result).b.data = element.b.data;
        return agg_2e_result;
}
bool l_class_OC_Fifo1_OC_1::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_1::run()
{
}
