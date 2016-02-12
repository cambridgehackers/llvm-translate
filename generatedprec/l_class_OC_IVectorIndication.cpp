#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication::heard(unsigned int heard_meth, unsigned long long heard_v_2e_coerce) {
        l_class_OC_FixedPoint v;
        stop_main_program = 1;
        v.data = heard_v_2e_coerce;
        ("Heard an ivector: %d %d\n")->(heard_meth, v.data);
}
bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
}
