#include "l_class_OC_EchoIndication.h"
void l_class_OC_EchoIndication__heard(l_class_OC_EchoIndication *thisp, unsigned int heard_v) {
        stop_main_program = 1;
        printf("Heard an echo: %d\n", heard_v);
}
bool l_class_OC_EchoIndication__heard__RDY(l_class_OC_EchoIndication *thisp) {
        return 1;
}
void l_class_OC_EchoIndication::run()
{
}
