#include "l_class_OC_IVector.h"
void l_class_OC_IVector__respond(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        BITS6 agg_2e_tmp;
        BITS4 agg_2e_tmp4;
        unsigned long long call;
        unsigned long long call_2e_i;
        unsigned long long call_2e_i_2e_1;
        l_struct_OC_ValueType temp;
        call_2e_i = temp.a;
        agg_2e_tmp = call_2e_i;
        call_2e_i_2e_1 = temp.b;
        agg_2e_tmp4 = call_2e_i_2e_1;
        call = thisp->gcounter;
        thisp->gcounter = call + 1;
        temp = thisp->fifo8.out.first();
        thisp->fifo8.out.deq();
        thisp->ind->heard(agg_2e_tmp, agg_2e_tmp4);
}
bool l_class_OC_IVector__respond__RDY(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->fifo8.out.first__RDY();
        tmp__2 = thisp->fifo8.out.deq__RDY();
        tmp__3 = thisp->ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector__say(void *thisarg, BITS6 say_meth, BITS4 say_v) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        l_struct_OC_ValueType temp;
        temp.a = say_meth;
        temp.b = say_v;
        thisp->fifo8.in.enq(temp);
}
bool l_class_OC_IVector__say__RDY(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->fifo8.in.enq__RDY();
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
