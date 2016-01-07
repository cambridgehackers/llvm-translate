#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication::heard(unsigned int heard_v) {
        stop_main_program = 1;
        ("Heard an ivector: %d %d\n")->(heard_v, 0);
}
bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
}
