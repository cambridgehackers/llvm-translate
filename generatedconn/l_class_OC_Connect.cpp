#include "l_class_OC_Connect.h"
void l_class_OC_Connect::respond(void) {
        BITS6 agg_2e_tmp;
        BITS4 agg_2e_tmp3;
        unsigned long long call;
        l_struct_OC_ValueType temp;
        call = gcounter;
        gcounter = call + 1;
        temp = fifo.out_first();
        fifo.out_deq();
        agg_2e_tmp = (temp.a);
        agg_2e_tmp3 = (temp.b);
        ind->heard(agg_2e_tmp, agg_2e_tmp3);
}
bool l_class_OC_Connect::respond__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo.out_first__RDY();
        tmp__2 = fifo.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Connect::say(BITS6 say_meth, BITS4 say_v) {
        l_struct_OC_ValueType temp;
        temp.a = (say_meth);
        temp.b = (say_v);
        fifo.in_enq(temp);
}
bool l_class_OC_Connect::say__RDY(void) {
        bool tmp__1;
        tmp__1 = fifo.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_Connect::run()
{
    if (respond__RDY()) respond();
    fifo.run();
    top.run();
}
