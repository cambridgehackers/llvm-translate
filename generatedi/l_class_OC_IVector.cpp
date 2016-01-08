#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond_rule(void) {
        l_struct_OC_ValuePair agg_2e_tmp;
        agg_2e_tmp = fifo.out_first();
        fifo.out_deq();
        ind->heard(agg_2e_tmp);
}
bool l_class_OC_IVector::respond_rule__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo.out_first__RDY();
        tmp__2 = fifo.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::say(l_struct_OC_ValuePair say_v) {
        fifo.in_enq(say_v);
}
bool l_class_OC_IVector::say__RDY(void) {
        bool tmp__1;
        tmp__1 = fifo.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_IVector::run()
{
    if (respond_rule__RDY()) respond_rule();
    fifo.run();
}
