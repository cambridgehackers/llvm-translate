#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond_rule(void) {
        l_struct_OC_ValuePair temp;
        temp = fifo1.out_first();
        fifo1.out_deq();
        ind->heard(temp.a, temp.b);
}
bool l_class_OC_IVector::respond_rule__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo1.out_first__RDY();
        tmp__2 = fifo1.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::say(unsigned int say_meth, unsigned int say_v) {
        l_struct_OC_ValuePair temp;
        temp.a = say_meth;
        temp.b = say_v;
        fifo1.in_enq(temp);
}
bool l_class_OC_IVector::say__RDY(void) {
        bool tmp__1;
        tmp__1 = fifo1.in_enq__RDY();
        return tmp__1;
}
void l_class_OC_IVector::run()
{
    if (respond_rule__RDY()) respond_rule();
    fifo0.run();
    fifo1.run();
    fifo2.run();
    fifo3.run();
    fifo4.run();
    fifo5.run();
    fifo6.run();
    fifo7.run();
    fifo8.run();
    fifo9.run();
    fifo10.run();
}
