#include "l_class_OC_IVector.h"
void l_class_OC_IVector__respond(l_class_OC_IVector *thisp) {
        BITS6 agg_2e_tmp;
        BITS4 agg_2e_tmp3;
        unsigned long long call;
        l_struct_OC_ValueType temp;
        call = thisp->gcounter;
        thisp->gcounter = call + 1;
        temp = thisp->fifo.out_first();
        thisp->fifo.out_deq();
        agg_2e_tmp->(temp.a);
        agg_2e_tmp3->(temp.b);
        thisp->ind->heard(agg_2e_tmp, agg_2e_tmp3);
}
bool l_class_OC_IVector__respond__RDY(l_class_OC_IVector *thisp) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->fifo.out_first__RDY();
        tmp__2 = thisp->fifo.out_deq__RDY();
        tmp__3 = thisp->ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector__say(l_class_OC_IVector *thisp, BITS6 say_meth, BITS4 say_v) {
        l_struct_OC_ValueType temp;
        temp.a.(say_meth);
        temp.b.(say_v);
        thisp->fifo.in_enq(temp);
}
bool l_class_OC_IVector__say__RDY(l_class_OC_IVector *thisp) {
        bool tmp__1;
        tmp__1 = thisp->fifo.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_IVector::run()
{
    if (respond__RDY()) respond();
    fifo.run();
    commit();
}
void l_class_OC_IVector::commit()
{
    fifo.commit();
}
