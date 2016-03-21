#include "l_class_OC_Lpm.h"
void l_class_OC_Lpm__respond(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp = thisp->fifo8.out.first();
        thisp->fifo8.out.deq();
        printf("respond: (%d, %d)\n", temp.a, temp.b);
        thisp->indication->heard(temp.a, temp.b);
}
bool l_class_OC_Lpm__respond__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->fifo8.out.first__RDY();
        tmp__2 = thisp->fifo8.out.deq__RDY();
        tmp__3 = thisp->indication->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Lpm__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp.a = say_meth;
        temp.b = say_v;
        printf("[%s:%d] (%d, %d)\n", ("say"), 55, say_meth, say_v);
        thisp->fifo8.in.enq(temp);
}
bool l_class_OC_Lpm__say__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->fifo8.in.enq__RDY();
        return tmp__1;
}
void l_class_OC_Lpm::run()
{
    if (respond__RDY()) respond();
    fifo.run();
    commit();
}
void l_class_OC_Lpm::commit()
{
    fifo.commit();
}
