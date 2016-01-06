#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong::in_enq(unsigned int in_enq_v) {
        if (pong)
            element2.in_enq(in_enq_v);
        if (pong ^ 1)
            element1.in_enq(in_enq_v);
        return ;
}
bool l_class_OC_FifoPong::in_enq__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = element2.in_enq__RDY();
        tmp__2 = element1.in_enq__RDY();
        return (tmp__1 | (pong ^ 1)) & (tmp__2 | pong);
}
void l_class_OC_FifoPong::out_deq(void) {
        pong = pong ^ 1;
        if (pong)
            element2.out_deq();
        if (pong ^ 1)
            element1.out_deq();
        return ;
}
bool l_class_OC_FifoPong::out_deq__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = element2.out_deq__RDY();
        tmp__2 = element1.out_deq__RDY();
        return (tmp__1 | (pong ^ 1)) & (tmp__2 | pong);
}
unsigned int l_class_OC_FifoPong::out_first(void) {
        unsigned int call;
        unsigned int call3;
        if (pong)
            call = element2.out_first();
        if (pong ^ 1)
            call3 = element1.out_first();
        return pong ? call:call3;
}
bool l_class_OC_FifoPong::out_first__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = element2.out_first__RDY();
        tmp__2 = element1.out_first__RDY();
        return (tmp__1 | (pong ^ 1)) & (tmp__2 | pong);
}
void l_class_OC_FifoPong::run()
{
    element1.run();
    element2.run();
}
