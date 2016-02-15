#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication::heard(bool heard_meth_2e_coerce, bool heard_v_2e_coerce) {
        l_class_OC_FixedPoint meth;
        l_class_OC_FixedPoint_OC_0 v;
        meth.data = heard_meth_2e_coerce;
        stop_main_program = 1;
        v.data = heard_v_2e_coerce;
        ("Heard an ivector: %d %d\n")->(*(meth.data), *(v.data));
}
bool l_class_OC_IVectorIndication::heard__RDY(void) {
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
}
