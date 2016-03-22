#include "l_class_OC_FifoPong.h"
void l_class_OC_FifoPong__deq(void *thisarg) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        thisp->pong_shadow = (thisp->pong) ^ 1;
        thisp->pong_valid = 1;
        if (thisp->pong)
            thisp->element2.out.deq();
        if ((thisp->pong) ^ 1)
            thisp->element1.out.deq();
}
bool l_class_OC_FifoPong__deq__RDY(void *thisarg) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.out.deq__RDY();
        tmp__2 = thisp->element1.out.deq__RDY();
        return (tmp__1 | ((thisp->pong) ^ 1)) & (tmp__2 | (thisp->pong));
}
void l_class_OC_FifoPong__enq(void *thisarg, l_struct_OC_ValuePair enq_v) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        if (thisp->pong)
            thisp->element2.in.enq(enq_v);
        if ((thisp->pong) ^ 1)
            thisp->element1.in.enq(enq_v);
}
bool l_class_OC_FifoPong__enq__RDY(void *thisarg) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.in.enq__RDY();
        tmp__2 = thisp->element1.in.enq__RDY();
        return (tmp__1 | ((thisp->pong) ^ 1)) & (tmp__2 | (thisp->pong));
}
l_struct_OC_ValuePair l_class_OC_FifoPong__first(void *thisarg) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        l_struct_OC_ValuePair first;
        if (thisp->pong)
            first = thisp->element2.out.first();
        if ((thisp->pong) ^ 1)
            first = thisp->element1.out.first();
        return first;
}
bool l_class_OC_FifoPong__first__RDY(void *thisarg) {
        l_class_OC_FifoPong * thisp = (l_class_OC_FifoPong *)thisarg;
        bool tmp__1;
        bool tmp__2;
        tmp__1 = thisp->element2.out.first__RDY();
        tmp__2 = thisp->element1.out.first__RDY();
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
