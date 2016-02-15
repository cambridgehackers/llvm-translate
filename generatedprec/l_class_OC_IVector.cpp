#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond(void) {
        BITS agg_2e_tmp;
        BITS agg_2e_tmp3;
        l_struct_OC_ValueType temp;
        agg_2e_tmp = temp.a;
        agg_2e_tmp3 = temp.b;
        temp = fifo.out_first();
        fifo.out_deq();
        ind->heard(agg_2e_tmp, agg_2e_tmp3);
        agg_2e_tmp3->~FixedPoint();
        agg_2e_tmp->~FixedPoint();
        temp->();
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
void l_class_OC_IVector::say(BITS say_meth, BITS say_v) {
        l_struct_OC_ValueType temp;
        temp.a = say_meth;
        temp.b = say_v;
        temp->();
        fifo.in_enq(temp);
        temp->();
        temp->();
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
    counter.run();
    gcounter.run();
}
