#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong::in_enq(unsigned int in_enq_v) {
        if (pong ^ 1)
            element1 = in_enq_v;
        if (pong)
            element2 = in_enq_v;
        full = 1;
        return ;
}
bool l_class_OC_FifoPong::in_enq__RDY(void) {
        return full ^ 1;
}
void l_class_OC_FifoPong::out_deq(void) {
        full = 0;
        pong = pong ^ 1;
}
bool l_class_OC_FifoPong::out_deq__RDY(void) {
        return full;
}
unsigned int l_class_OC_FifoPong::out_first(void) {
        return pong ? element2:element1;
}
bool l_class_OC_FifoPong::out_first__RDY(void) {
        return full;
}
void l_class_OC_FifoPong::run()
{
}
