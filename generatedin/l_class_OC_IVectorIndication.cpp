#include "l_class_OC_IVectorIndication.h"
void l_class_OC_IVectorIndication__heard(void *thisarg, unsigned int heard_meth, unsigned int heard_v) {
        l_class_OC_IVectorIndication * thisp = (l_class_OC_IVectorIndication *)thisarg;
        printf("Heard an ivector: %d %d\n", heard_meth, heard_v);
}
bool l_class_OC_IVectorIndication__heard__RDY(void *thisarg) {
        l_class_OC_IVectorIndication * thisp = (l_class_OC_IVectorIndication *)thisarg;
        return 1;
}
void l_class_OC_IVectorIndication::run()
{
    commit();
}
void l_class_OC_IVectorIndication::commit()
{
}
