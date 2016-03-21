#include "l_class_OC_Lpm.h"
void l_class_OC_Lpm__enter(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp = thisp->inQ8.out.first();
        printf("enter: (%d, %d)\n", temp.a, temp.b);
        thisp->inQ8.out.deq();
        thisp->fifo8.in.enq(temp);
        thisp->mem.req(temp);
}
bool l_class_OC_Lpm__enter__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        tmp__1 = thisp->inQ8.out.first__RDY();
        tmp__2 = thisp->inQ8.out.deq__RDY();
        tmp__3 = thisp->fifo8.in.enq__RDY();
        tmp__4 = thisp->mem.req__RDY();
        return ((tmp__1 & tmp__2) & tmp__3) & tmp__4;
}
void l_class_OC_Lpm__exit(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair mtemp;
        l_struct_OC_ValuePair temp;
        temp = thisp->fifo8.out.first();
        mtemp = thisp->mem.resValue();
        thisp->mem.resAccept();
        thisp->fifo8.out.deq();
        printf("exit: (%d, %d)\n", temp.a, temp.b);
        thisp->outQ8.in.enq(temp);
}
bool l_class_OC_Lpm__exit__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        bool tmp__5;
        thisp->doneCount_shadow = (thisp->doneCount) + 1;
        thisp->doneCount_valid = 1;
        tmp__1 = thisp->fifo8.out.first__RDY();
        tmp__2 = thisp->mem.resValue__RDY();
        tmp__3 = thisp->mem.resAccept__RDY();
        tmp__4 = thisp->fifo8.out.deq__RDY();
        tmp__5 = thisp->outQ8.in.enq__RDY();
        printf("[%s:%d] done [%d] = %d\n", ("done"), 79, thisp->doneCount, (((thisp->doneCount) % 5) != 0) ^ 1);
        return (((tmp__1 & tmp__2) & tmp__3) & tmp__4) & tmp__5;
}
void l_class_OC_Lpm__recirc(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair mtemp;
        l_struct_OC_ValuePair temp;
        temp = thisp->fifo8.out.first();
        mtemp = thisp->mem.resValue();
        thisp->mem.resAccept();
        thisp->fifo8.out.deq();
        printf("recirc: (%d, %d)\n", temp.a, temp.b);
        thisp->fifo8.in.enq(temp);
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
        thisp->doneCount_shadow = (thisp->doneCount) + 1;
        thisp->doneCount_valid = 1;
        tmp__1 = thisp->fifo8.out.first__RDY();
        tmp__2 = thisp->mem.resValue__RDY();
        tmp__3 = thisp->mem.resAccept__RDY();
        tmp__4 = thisp->fifo8.out.deq__RDY();
        tmp__5 = thisp->fifo8.in.enq__RDY();
        tmp__6 = thisp->mem.req__RDY();
        printf("[%s:%d] done [%d] = %d\n", ("done"), 79, thisp->doneCount, (((thisp->doneCount) % 5) != 0) ^ 1);
        return ((((((1 ^ 1) & tmp__1) & tmp__2) & tmp__3) & tmp__4) & tmp__5) & tmp__6;
}
void l_class_OC_Lpm__respond(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp = thisp->outQ8.out.first();
        thisp->outQ8.out.deq();
        printf("respond: (%d, %d)\n", temp.a, temp.b);
        thisp->indication->heard(temp.a, temp.b);
}
bool l_class_OC_Lpm__respond__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = thisp->outQ8.out.first__RDY();
        tmp__2 = thisp->outQ8.out.deq__RDY();
        tmp__3 = thisp->indication->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Lpm__say(void *thisarg, unsigned int say_meth, unsigned int say_v) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        l_struct_OC_ValuePair temp;
        temp.a = say_meth;
        temp.b = say_v;
        printf("[%s:%d] (%d, %d)\n", ("say"), 71, say_meth, say_v);
        thisp->inQ8.in.enq(temp);
}
bool l_class_OC_Lpm__say__RDY(void *thisarg) {
        l_class_OC_Lpm * thisp = (l_class_OC_Lpm *)thisarg;
        bool tmp__1;
        tmp__1 = thisp->inQ8.in.enq__RDY();
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
