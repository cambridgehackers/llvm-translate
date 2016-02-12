#include "l_class_OC_IVector.h"
void l_class_OC_IVector::respond0(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo0.out_first();
        fifo0.out_deq();
        ind->heard(0, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond0__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo0.out_first__RDY();
        tmp__2 = fifo0.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond1(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo1.out_first();
        fifo1.out_deq();
        ind->heard(1, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond1__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo1.out_first__RDY();
        tmp__2 = fifo1.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond2(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo2.out_first();
        fifo2.out_deq();
        ind->heard(2, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond2__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo2.out_first__RDY();
        tmp__2 = fifo2.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond3(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo3.out_first();
        fifo3.out_deq();
        ind->heard(3, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond3__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo3.out_first__RDY();
        tmp__2 = fifo3.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond4(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo4.out_first();
        fifo4.out_deq();
        ind->heard(4, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond4__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo4.out_first__RDY();
        tmp__2 = fifo4.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond5(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo5.out_first();
        fifo5.out_deq();
        ind->heard(5, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond5__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo5.out_first__RDY();
        tmp__2 = fifo5.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond6(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo6.out_first();
        fifo6.out_deq();
        ind->heard(6, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond6__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo6.out_first__RDY();
        tmp__2 = fifo6.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond7(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo7.out_first();
        fifo7.out_deq();
        ind->heard(7, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond7__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo7.out_first__RDY();
        tmp__2 = fifo7.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond8(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo8.out_first();
        fifo8.out_deq();
        ind->heard(8, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond8__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo8.out_first__RDY();
        tmp__2 = fifo8.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::respond9(void) {
        l_class_OC_FixedPoint agg_2e_tmp;
        l_struct_OC_ValuePair temp;
        agg_2e_tmp = temp.b;
        temp = fifo9.out_first();
        fifo9.out_deq();
        ind->heard(9, agg_2e_tmp.data);
}
bool l_class_OC_IVector::respond9__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo9.out_first__RDY();
        tmp__2 = fifo9.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_IVector::say(unsigned int say_meth, unsigned long long say_v_2e_coerce) {
        l_struct_OC_ValuePair temp;
        l_class_OC_FixedPoint v;
        temp.b = v;
        v.data = say_v_2e_coerce;
        temp->ValuePair();
        (say_meth == 0 ? &fifo0:say_meth == 1 ? &fifo1:say_meth == 2 ? &fifo2:say_meth == 3 ? &fifo3:say_meth == 4 ? &fifo4:say_meth == 5 ? &fifo5:say_meth == 6 ? &fifo6:say_meth == 7 ? &fifo7:say_meth == 8 ? &fifo8:&fifo9)->in_enq(temp);
        return ;
}
bool l_class_OC_IVector::say__RDY(void) {
        bool tmp__1;
        bool tmp__10;
        bool tmp__2;
        bool tmp__3;
        bool tmp__4;
        bool tmp__5;
        bool tmp__6;
        bool tmp__7;
        bool tmp__8;
        bool tmp__9;
        tmp__1 = fifo0.in_enq__RDY();
        tmp__10 = fifo9.in_enq__RDY();
        tmp__2 = fifo1.in_enq__RDY();
        tmp__3 = fifo2.in_enq__RDY();
        tmp__4 = fifo3.in_enq__RDY();
        tmp__5 = fifo4.in_enq__RDY();
        tmp__6 = fifo5.in_enq__RDY();
        tmp__7 = fifo6.in_enq__RDY();
        tmp__8 = fifo7.in_enq__RDY();
        tmp__9 = fifo8.in_enq__RDY();
        return ((((((((tmp__1 & tmp__2) & tmp__3) & tmp__4) & tmp__5) & tmp__6) & tmp__7) & tmp__8) & tmp__9) & tmp__10;
}
void l_class_OC_IVector::run()
{
    if (respond0__RDY()) respond0();
    if (respond1__RDY()) respond1();
    if (respond2__RDY()) respond2();
    if (respond3__RDY()) respond3();
    if (respond4__RDY()) respond4();
    if (respond5__RDY()) respond5();
    if (respond6__RDY()) respond6();
    if (respond7__RDY()) respond7();
    if (respond8__RDY()) respond8();
    if (respond9__RDY()) respond9();
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
