#include "l_class_OC_Lpm.h"
void l_class_OC_Lpm__enter(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp = thisp->inQ.out.first();
        printf("enter: (%d, %d)\n", temp.a, temp.b);
        thisp->inQ.out.deq();
        thisp->fifo.in.enq(temp);
        thisp->mem.req(temp);
}
bool l_class_OC_Lpm__enter__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        tmp__1 = thisp->inQ.out.first__RDY();
        tmp__2 = thisp->inQ.out.deq__RDY();
        tmp__3 = thisp->fifo.in.enq__RDY();
        tmp__4 = thisp->mem.req__RDY();
        return ((tmp__1 & tmp__2) & tmp__3) & tmp__4;
}
void l_class_OC_Lpm__exit(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair mtemp;
        l_struct_OC_ValuePair temp;
        temp = thisp->fifo.out.first();
        mtemp = thisp->mem.resValue();
        thisp->mem.resAccept();
        thisp->fifo.out.deq();
        printf("exit: (%d, %d)\n", temp.a, temp.b);
        thisp->outQ.in.enq(temp);
}
bool l_class_OC_Lpm__exit__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        bool tmp__5;
        tmp__1 = thisp->fifo.out.first__RDY();
        tmp__2 = thisp->mem.resValue__RDY();
        tmp__3 = thisp->mem.resAccept__RDY();
        tmp__4 = thisp->fifo.out.deq__RDY();
        tmp__5 = thisp->outQ.in.enq__RDY();
        return (((tmp__1 & tmp__2) & tmp__3) & tmp__4) & tmp__5;
}
void l_class_OC_Lpm__recirc(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair mtemp;
        l_struct_OC_ValuePair temp;
        temp = thisp->fifo.out.first();
        mtemp = thisp->mem.resValue();
        thisp->mem.resAccept();
        thisp->fifo.out.deq();
        printf("recirc: (%d, %d)\n", temp.a, temp.b);
        thisp->fifo.in.enq(temp);
        thisp->mem.req(temp);
}
bool l_class_OC_Lpm__recirc__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        bool tmp__5;
        bool tmp__6;
        tmp__1 = thisp->fifo.out.first__RDY();
        tmp__2 = thisp->mem.resValue__RDY();
        tmp__3 = thisp->mem.resAccept__RDY();
        tmp__4 = thisp->fifo.out.deq__RDY();
        tmp__5 = thisp->fifo.in.enq__RDY();
        tmp__6 = thisp->mem.req__RDY();
        return ((((tmp__1 & tmp__2) & tmp__3) & tmp__4) & tmp__5) & tmp__6;
}
void l_class_OC_Lpm__respond(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp = thisp->outQ.out.first();
        thisp->outQ.out.deq();
        printf("respond: (%d, %d)\n", temp.a, temp.b);
        thisp->indication->heard(temp.a, temp.b);
}
bool l_class_OC_Lpm__respond__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->outQ.out.first__RDY();
        tmp__2 = thisp->outQ.out.deq__RDY();
        tmp__3 = thisp->indication->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Lpm__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp.a = say_meth;
        temp.b = say_v;
        printf("[%s:%d] (%d, %d)\n", ("say"), 99, say_meth, say_v);
        thisp->inQ.in.enq(temp);
}
bool l_class_OC_Lpm__say__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->inQ.in.enq__RDY();
        return tmp__1;
}
void l_class_OC_Lpm::run()
{
    if (enter__RDY()) enter();
    if (exit__RDY()) exit();
    if (recirc__RDY()) recirc();
    if (respond__RDY()) respond();
    inQ.run();
    fifo.run();
    outQ.run();
    mem.run();
    commit();
}
void l_class_OC_Lpm::commit()
{
    if (doneCount_valid) doneCount = doneCount_shadow;
    doneCount_valid = 0;
    inQ.commit();
    fifo.commit();
    outQ.commit();
    mem.commit();
}
