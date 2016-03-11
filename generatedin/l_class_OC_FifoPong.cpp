#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong__in_enq(l_class_OC_FifoPong *thisp, l_struct_OC_ValuePair in_enq_v) {
        if (thisp->pong)
            thisp->element2.in_enq(in_enq_v);
        if ((thisp->pong) ^ 1)
            thisp->element1.in_enq(in_enq_v);
}
bool l_class_OC_FifoPong__in_enq__RDY(l_class_OC_FifoPong *thisp) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.in_enq__RDY();
        tmp__2 = thisp->element1.in_enq__RDY();
        return (tmp__1 | ((thisp->pong) ^ 1)) & (tmp__2 | (thisp->pong));
}
void l_class_OC_FifoPong__out_deq(l_class_OC_FifoPong *thisp) {
        thisp->pong_shadow = (thisp->pong) ^ 1;
        thisp->pong_valid = 1;
        if (thisp->pong)
            thisp->element2.out_deq();
        if ((thisp->pong) ^ 1)
            thisp->element1.out_deq();
}
bool l_class_OC_FifoPong__out_deq__RDY(l_class_OC_FifoPong *thisp) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.out_deq__RDY();
        tmp__2 = thisp->element1.out_deq__RDY();
        return (tmp__1 | ((thisp->pong) ^ 1)) & (tmp__2 | (thisp->pong));
}
l_struct_OC_ValuePair l_class_OC_FifoPong__out_first(l_class_OC_FifoPong *thisp) {
        l_struct_OC_ValuePair out_first;
        if (thisp->pong)
            out_first = thisp->element2.out_first();
        if ((thisp->pong) ^ 1)
            out_first = thisp->element1.out_first();
        return out_first;
}
bool l_class_OC_FifoPong__out_first__RDY(l_class_OC_FifoPong *thisp) {
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.out_first__RDY();
        tmp__2 = thisp->element1.out_first__RDY();
        return (tmp__1 | ((thisp->pong) ^ 1)) & (tmp__2 | (thisp->pong));
}
void l_class_OC_FifoPong::run()
{
    element1.run();
    element2.run();
    commit();
}
void l_class_OC_FifoPong::commit()
{
    if (pong_valid) pong = pong_shadow;
    pong_valid = 0;
    element1.commit();
    element2.commit();
}
