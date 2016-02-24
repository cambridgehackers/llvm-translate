#include "l_class_OC_ConnectIndication.h"
void l_class_OC_ConnectIndication::heard(unsigned int heard_meth, unsigned int heard_v) {
        stop_main_program = 1;
        printf("Heard an connect: %d %d\n", 0, 0);
}
bool l_class_OC_ConnectIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_ConnectIndication::run()
{
}
