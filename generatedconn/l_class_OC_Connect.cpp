#include "l_class_OC_Connect.h"
void l_class_OC_Connect::respond(void) {
        l_struct_OC_ValueType temp;
        temp = fifo.out_first();
        fifo.out_deq();
        ind->heard(temp.a, temp.b);
}
bool l_class_OC_Connect::respond__RDY(void) {
        bool tmp__1;
        bool tmp__2;
        bool tmp__3;
        tmp__1 = fifo.out_first__RDY();
        tmp__2 = fifo.out_deq__RDY();
        tmp__3 = ind->heard__RDY();
        return (tmp__1 & tmp__2) & tmp__3;
}
void l_class_OC_Connect::say(unsigned int say_meth, unsigned int say_v) {
        lEchoRequestOutput_test.request_say(say_meth, say_v);
}
bool l_class_OC_Connect::say__RDY(void) {
        bool tmp__1;
        tmp__1 = lEchoRequestOutput_test.request_say__RDY();
        return tmp__1;
}
void l_class_OC_Connect::run()
{
    if (respond__RDY()) respond();
    fifo.run();
    lEchoIndicationOutput.run();
    lEchoRequestInput.run();
    lEcho.run();
    lEchoRequestOutput_test.run();
    lEchoIndicationInput_test.run();
}
