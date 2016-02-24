#include "l_class_OC_Connect.h"
void l_class_OC_Connect::say(unsigned int say_meth, unsigned int say_v) {
        printf("entered Connect::say\n");
        lEchoRequestOutput_test.request_say(say_meth, say_v);
}
bool l_class_OC_Connect::say__RDY(void) {
        bool tmp__1;
        tmp__1 = lEchoRequestOutput_test.request_say__RDY();
        return tmp__1;
}
void l_class_OC_Connect::run()
{
    lEchoIndicationOutput.run();
    lEchoRequestInput.run();
    lEcho.run();
    lEchoRequestOutput_test.run();
    lEchoIndicationInput_test.run();
}
