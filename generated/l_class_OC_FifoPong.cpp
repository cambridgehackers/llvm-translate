#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong__in_enq(l_class_OC_FifoPong *thisp, unsigned int in_enq_v) {
        if ((thisp->pong) ^ 1)
            thisp->element1 = in_enq_v;
        if (thisp->pong)
            thisp->element2 = in_enq_v;
        thisp->full = 1;
}
bool l_class_OC_FifoPong__in_enq__RDY(l_class_OC_FifoPong *thisp) {
        return (thisp->full) ^ 1;
}
void l_class_OC_FifoPong__out_deq(l_class_OC_FifoPong *thisp) {
        thisp->full = 0;
        thisp->pong = (thisp->pong) ^ 1;
}
bool l_class_OC_FifoPong__out_deq__RDY(l_class_OC_FifoPong *thisp) {
        return thisp->full;
}
unsigned int l_class_OC_FifoPong__out_first(l_class_OC_FifoPong *thisp) {
        return thisp->pong ? thisp->element2:thisp->element1;
}
bool l_class_OC_FifoPong__out_first__RDY(l_class_OC_FifoPong *thisp) {
        return thisp->full;
}
void l_class_OC_FifoPong::run()
{
}
