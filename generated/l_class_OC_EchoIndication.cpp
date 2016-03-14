#include "l_class_OC_EchoIndication.h"
void l_class_OC_EchoIndication__heard(void *thisarg, unsigned int heard_v) {
        l_class_OC_EchoIndication * thisp = (l_class_OC_EchoIndication *)thisarg;
        stop_main_program = 1;
        printf("Heard an echo: %d\n", heard_v);
}
bool l_class_OC_EchoIndication__heard__RDY(void *thisarg) {
        l_class_OC_EchoIndication * thisp = (l_class_OC_EchoIndication *)thisarg;
        return 1;
}
void l_class_OC_EchoIndication::run()
{
    commit();
}
void l_class_OC_EchoIndication::commit()
{
}
