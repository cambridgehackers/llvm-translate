#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1::in_enq(l_struct_OC_ValueType in_enq_v) {
        unsigned long long call_2e_i_2e_1_2e_i;
        unsigned long long call_2e_i_2e_i;
        call_2e_i_2e_1_2e_i = in_enq_v.b;
        call_2e_i_2e_i = in_enq_v.a;
        element.a = call_2e_i_2e_i;
        element.b = call_2e_i_2e_1_2e_i;
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
        unsigned long long call_2e_i_2e_1_2e_i;
        unsigned long long call_2e_i_2e_i;
        l_struct_OC_ValueType out_first;
        call_2e_i_2e_1_2e_i = element.b;
        call_2e_i_2e_i = element.a;
        out_first.a = call_2e_i_2e_i;
        out_first.b = call_2e_i_2e_1_2e_i;
        return out_first;
}
bool l_class_OC_Fifo1_OC_1::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_1::run()
{
}
