#include "l_class_OC_LpmIndication.h"
void l_class_OC_LpmIndication__heard(void *thisarg, unsigned int heard_meth, unsigned int heard_v) {
        l_class_OC_LpmIndication * thisp = (l_class_OC_LpmIndication *)thisarg;
        stop_main_program = 1;
        printf("Heard an lpm: %d %d\n", heard_meth, heard_v);
}
bool l_class_OC_LpmIndication__heard__RDY(void *thisarg) {
        l_class_OC_LpmIndication * thisp = (l_class_OC_LpmIndication *)thisarg;
        return 1;
}
void l_class_OC_LpmIndication::run()
{
    commit();
}
void l_class_OC_LpmIndication::commit()
{
}
