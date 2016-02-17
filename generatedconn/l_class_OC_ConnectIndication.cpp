#include "l_class_OC_ConnectIndication.h"
void l_class_OC_ConnectIndication::heard(BITS6 heard_meth, BITS4 heard_v) {
        stop_main_program = 1;
        ("Heard an connect: %d %d\n")->(0, 0);
}
bool l_class_OC_ConnectIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_ConnectIndication::run()
{
}
