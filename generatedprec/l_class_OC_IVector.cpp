#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond(void) {
        BITS6 agg_2e_tmp;
        BITS4 agg_2e_tmp3;
        l_struct_OC_ValueType temp;
        temp = fifo.out_first();
        fifo.out_deq();
        agg_2e_tmp->FixedPoint(temp.a);
        agg_2e_tmp3->FixedPoint(temp.b);
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
void l_class_OC_IVector::say(BITS6 say_meth, BITS4 say_v) {
        l_struct_OC_ValueType agg_2e_tmp;
        l_struct_OC_ValueType temp;
        temp.a.data = (say_meth)->data;
        temp.b.data = (say_v)->data;
        temp->();
        agg_2e_tmp->(temp);
        fifo.in_enq(agg_2e_tmp);
        agg_2e_tmp->();
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
}
