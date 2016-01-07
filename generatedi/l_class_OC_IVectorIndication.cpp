#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication::heard(l_struct_OC_ValuePair heard_v) {
        stop_main_program = 1;
        ("Heard an ivector: %d %d\n")->(heard_v->a, heard_v->b);
}
bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
}
