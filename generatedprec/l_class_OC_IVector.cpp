#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond(void) {
        BITS6 agg_2e_tmp;
        BITS4 agg_2e_tmp3;
        unsigned long long call;
        unsigned long long call_2e_i;
        unsigned long long call_2e_i_2e_1;
        l_struct_OC_ValueType temp;
        agg_2e_tmp = call_2e_i;
        agg_2e_tmp3 = call_2e_i_2e_1;
        call = gcounter;
        call_2e_i = temp.a;
        call_2e_i_2e_1 = temp.b;
        gcounter = call + 1;
        temp = fifo.out_first();
        fifo.out_deq();
        ind->heard(agg_2e_tmp, agg_2e_tmp3);
}
bool l_class_OC_IVector::respond__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo.out_first__RDY();
        tmp__2 = fifo.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::say(BITS6 say_meth, BITS4 say_v) {
        l_struct_OC_ValueType agg_2e_tmp;
        unsigned long long call_2e_i;
        unsigned long long call_2e_i_2e_1;
        unsigned long long call_2e_i_2e_1_2e_i;
        unsigned long long call_2e_i_2e_i;
        l_struct_OC_ValueType temp;
        agg_2e_tmp.a = call_2e_i_2e_i;
        agg_2e_tmp.b = call_2e_i_2e_1_2e_i;
        call_2e_i = say_meth;
        call_2e_i_2e_1 = say_v;
        call_2e_i_2e_1_2e_i = temp.b;
        call_2e_i_2e_i = temp.a;
        temp.a = call_2e_i;
        temp.b = call_2e_i_2e_1;
        fifo.in_enq(agg_2e_tmp);
}
bool l_class_OC_IVector::say__RDY(void) {
        bool tmp__1;
        tmp__1 = fifo.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_IVector::run()
{
    if (respond__RDY()) respond();
    fifo.run();
}
