#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_class_OC_FixedPoint_OC_2 agg_2e_tmp3;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.a;
        agg_2e_tmp3 = temp.b;
        temp = fifo.out_first();
        fifo.out_deq();
        ind->heard(agg_2e_tmp.data, agg_2e_tmp3.data);
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
void l_class_OC_IVector::say(unsigned long long say_meth_2e_coerce, unsigned long long say_v_2e_coerce) {
        l_class_OC_FixedPoint meth;
        l_struct_OC_ValuePair temp;
        l_class_OC_FixedPoint_OC_2 v;
        meth.data = say_meth_2e_coerce;
        temp.a = meth;
        temp.b = v;
        v.data = say_v_2e_coerce;
        temp->ValuePair();
        fifo.in_enq(temp);
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
