#include "l_class_OC_Fifo1_OC_3.h"
void l_class_OC_Fifo1_OC_3::in_enq(class l_struct_OC_ValuePair *in_enq_v) {
        full = 1;
}
bool l_class_OC_Fifo1_OC_3::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_Fifo1_OC_3::out_deq(void) {
        full = 0;
}
bool l_class_OC_Fifo1_OC_3::out_deq__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_3::out_first(void) {
        agg_2e_result->(&element, 88, 4, 0);
}
bool l_class_OC_Fifo1_OC_3::out_first__RDY(void) {
        return full;
}
void l_class_OC_Fifo1_OC_3::run()
{
    element.run();
}