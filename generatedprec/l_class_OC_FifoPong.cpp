#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong::in_enq(l_struct_OC_ValuePair in_enq_v) {
        element1.in_enq(in_enq_v);
}
bool l_class_OC_FifoPong::in_enq__RDY(void) {
        bool tmp__1;
        tmp__1 = element1.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_FifoPong::out_deq(void) {
        element1.out_deq();
}
bool l_class_OC_FifoPong::out_deq__RDY(void) {
        bool tmp__1;
        tmp__1 = element1.out_deq__RDY();
        return tmp__1;
}
l_struct_OC_ValuePair l_class_OC_FifoPong::out_first(void) {
        l_struct_OC_ValuePair agg_2e_result;
        agg_2e_result = element1.out_first();
}
bool l_class_OC_FifoPong::out_first__RDY(void) {
        bool tmp__1;
        tmp__1 = element1.out_first__RDY();
        return tmp__1;
}
void l_class_OC_FifoPong::run()
{
    element1.run();
}
