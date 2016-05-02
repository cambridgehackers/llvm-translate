#include "l_class_OC_IVector.h"
void l_class_OC_IVector__respond(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        l_struct_OC_ValueType call;
        unsigned long long call4;
        l_struct_OC_ValueType temp;
        call = thisp->fifo.out.first();
        temp.a = ((call.a) & (-1));
        temp.b = ((call.b) & (-1));
        call4 = thisp->gcounter;
        thisp->gcounter = call4 + 1;
        thisp->fifo.out.deq();
        thisp->ind->heard(temp.a, temp.b);
}
bool l_class_OC_IVector__respond__RDY(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->fifo.out.first__RDY();
        tmp__2 = thisp->fifo.out.deq__RDY();
        tmp__3 = thisp->ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector__say(void *thisarg, bool say_meth, bool say_v) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        l_struct_OC_ValueType temp;
        temp.a = ((say_meth) & (-1));
        temp.b = ((say_v) & (-1));
        thisp->fifo.in.enq(temp);
}
bool l_class_OC_IVector__say__RDY(void *thisarg) {
        l_class_OC_IVector * thisp = (l_class_OC_IVector *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->fifo.in.enq__RDY();
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
