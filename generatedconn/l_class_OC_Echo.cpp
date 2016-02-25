#include "l_class_OC_Echo.h"
void l_class_OC_Echo::say(unsigned int say_meth, unsigned int say_v) {
        indication->heard(say_meth, say_v);
}
bool l_class_OC_Echo::say__RDY(void) {
        bool tmp__1;
        tmp__1 = indication->heard__RDY();
        return tmp__1;
}
void l_class_OC_Echo::run()
{
}
