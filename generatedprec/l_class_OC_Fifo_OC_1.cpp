#include "l_class_OC_Fifo_OC_1.h"
void l_class_OC_Fifo_OC_1::in_enq(bool in_enq_v_2e_coerce0, bool in_enq_v_2e_coerce1) {
        l_struct_OC_ValueType v;
        (v)-> = in_enq_v_2e_coerce1;
}
bool l_class_OC_Fifo_OC_1::in_enq__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_1::out_deq(void) {
}
bool l_class_OC_Fifo_OC_1::out_deq__RDY(void) {
        return 0;
}
l_unnamed_2 l_class_OC_Fifo_OC_1::out_first(void) {
        l_struct_OC_ValueType retval;
        retval.a.FixedPoint();
        retval.b.FixedPoint();
        return *(retval);
}
bool l_class_OC_Fifo_OC_1::out_first__RDY(void) {
        return 0;
}
void l_class_OC_Fifo_OC_1::run()
{
}
