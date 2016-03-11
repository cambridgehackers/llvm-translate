#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication__heard(l_class_OC_IVectorIndication *thisp, unsigned int heard_meth, unsigned int heard_v) {
        stop_main_program = 1;
        printf("Heard an ivector: %d %d\n", heard_meth, heard_v);
}
bool l_class_OC_IVectorIndication__heard__RDY(l_class_OC_IVectorIndication *thisp) {
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
    commit();
}
void l_class_OC_IVectorIndication::commit()
{
}
