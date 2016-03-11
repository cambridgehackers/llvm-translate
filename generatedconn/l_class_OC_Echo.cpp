#include "l_class_OC_Echo.h"
void l_class_OC_Echo__say(l_class_OC_Echo *thisp, unsigned int say_meth, unsigned int say_v) {
        thisp->indication->heard(say_meth, say_v);
}
bool l_class_OC_Echo__say__RDY(l_class_OC_Echo *thisp) {
        bool tmp__1;
        tmp__1 = thisp->indication->heard__RDY();
        return tmp__1;
}
void l_class_OC_Echo::run()
{
    commit();
}
void l_class_OC_Echo::commit()
{
}
