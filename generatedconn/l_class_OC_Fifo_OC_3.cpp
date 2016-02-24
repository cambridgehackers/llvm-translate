#include "l_class_OC_Fifo_OC_3.h"
void l_class_OC_Fifo_OC_3::in_enq(l_struct_OC_ValueType in_enq_v) {
}
bool l_class_OC_Fifo_OC_3::in_enq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_3::out_deq(void) {
}
bool l_class_OC_Fifo_OC_3::out_deq__RDY(void) {
        return 0;
}
l_struct_OC_ValueType l_class_OC_Fifo_OC_3::out_first(void) {
        l_struct_OC_ValueType out_first;
        out_first.a = 0;
        out_first.b = 0;
        return out_first;
}
bool l_class_OC_Fifo_OC_3::out_first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_3::run()
{
}
