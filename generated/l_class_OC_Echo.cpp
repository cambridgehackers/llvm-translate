#include "l_class_OC_Echo.h"
void l_class_OC_Echo__respond_rule(l_class_OC_Echo *thisp) {
        unsigned int call;
        unsigned int temp;
        call = thisp->fifo.out_first();
        temp = call;
        thisp->fifo.out_deq();
        thisp->ind->heard(temp);
}
bool l_class_OC_Echo__respond_rule__RDY(l_class_OC_Echo *thisp) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->fifo.out_first__RDY();
        tmp__2 = thisp->fifo.out_deq__RDY();
        tmp__3 = thisp->ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_v) {
        thisp->fifo.in_enq(say_v);
}
bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp) {
        bool tmp__1;
        tmp__1 = thisp->fifo.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_Echo::run()
{
    if (respond_rule__RDY()) respond_rule();
    fifo.run();
}
