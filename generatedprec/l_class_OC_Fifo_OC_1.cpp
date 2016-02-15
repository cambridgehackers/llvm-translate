#include "l_class_OC_Fifo_OC_1.h"
void l_class_OC_Fifo_OC_1::in_enq(l_struct_OC_ValueType in_enq_v) {
}
bool l_class_OC_Fifo_OC_1::in_enq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_1::out_deq(void) {
}
bool l_class_OC_Fifo_OC_1::out_deq__RDY(void) {
        return 0;
}
l_struct_OC_ValueType l_class_OC_Fifo_OC_1::out_first(void) {
        l_struct_OC_ValueType agg_2e_result;
        (agg_2e_result)->a.FixedPoint();
        (agg_2e_result)->b.FixedPoint();
}
bool l_class_OC_Fifo_OC_1::out_first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_1::run()
{
}
