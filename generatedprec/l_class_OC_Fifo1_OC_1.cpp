#include "l_class_OC_Fifo1_OC_1.h"
void l_class_OC_Fifo1_OC_1::in_enq(l_struct_OC_ValueType in_enq_v) {
        element.a.data = in_enq_v.a.data;
        element.b.data = in_enq_v.b.data;
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
        l_struct_OC_ValueType out_first;
        out_first.a.data = element.a.data;
        out_first.b.data = element.b.data;
        return out_first;
}
bool l_class_OC_Fifo1_OC_1::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_1::run()
{
}
